module;
#include <nlohmann/json.hpp>
module storage.kernel.runtime;
import std;
import storage.kernel.load;
import storage.google;
import storage.http;
import storage.state;

namespace
{
    auto queue_path(std::string_view address) -> std::filesystem::path
    {
        return std::filesystem::path{"blob_queue"}/storage_digest(address);
    }
    auto clear_path(std::string_view address) -> std::filesystem::path
    {
        return "clear." + storage_digest(address);
    }
} // namespace
auto storage_service_accounts(const std::filesystem::path& directory)
    -> std::vector<service_account_file_t>
{
    std::vector<service_account_file_t> accounts;
    const auto registry = directory / "accounts.json";
    if (!std::filesystem::exists(registry)) return accounts;
    // Only successful in-app provisioning registers a key. Unrelated JSON
    // files dropped in key/ are intentionally never enumerated or opened.
    const auto managed = nlohmann::json::parse(storage_read(registry));
    for (const auto& [address,path] : managed.items())
        accounts.push_back({address,path.get<std::string>()});
    std::ranges::sort(accounts, [](const auto& a, const auto& b)
        { return std::tie(a.address,a.account_path) < std::tie(b.address,b.account_path); });
    return accounts;
}
storage_runtime_t::storage_runtime_t(std::atomic_bool* close) : m_close{close}
{
}
auto storage_runtime_t::open_space(const storage_settings_t& settings, const storage_space_t& space,
    const std::shared_ptr<request_control_t>& control) -> std::shared_ptr<storage_session_t>
{
    auto session = std::make_shared<storage_session_t>();
    session->storage_id = space.id;
    session->address = space.name;
    session->root_path = (std::filesystem::path{settings.root_path}/space.id).generic_string();
    const auto registered = storage_service_accounts();
    std::set<std::string> clearing_accounts;
    for (const auto& entry : std::filesystem::directory_iterator{"."})
        if (entry.is_regular_file() && entry.path().filename().string().starts_with("clear."))
        {
            const auto checkpoint=nlohmann::json::parse(storage_read(entry.path()));
            for (const auto& address : checkpoint.at("accounts"))
                clearing_accounts.insert(address.get<std::string>());
        }
    // Start queue workers in project rounds as well: association order must not
    // start every worker in one project before any worker in the next project.
    std::map<std::string,std::deque<std::string>> by_project;
    for (const auto& address : space.accounts) by_project[address.substr(address.find('@')+1)].push_back(address);
    std::vector<std::string> interleaved;
    while (interleaved.size()<space.accounts.size())
        for (auto& [project,addresses] : by_project)
            if (!addresses.empty()) { interleaved.push_back(addresses.front()); addresses.pop_front(); }
    for (const auto& address : interleaved)
    {
        const auto found = std::ranges::find(registered,address,&service_account_file_t::address);
        if (found==registered.end()) throw std::runtime_error{"無法讀取程式建立的服務帳戶："+address};
        const auto credential = nlohmann::json::parse(storage_read(found->account_path));
        std::lock_guard lock{m_mutex};
        auto& account = m_accounts[address];
        if (!account) account = std::make_shared<google_account_t>(address,credential.at("private_key").get<std::string>());
        auto& reserve = m_queues[address];
        if (!reserve) reserve = std::make_shared<blob_queue_t>(queue_path(address),account.get(),m_close);
        if (clearing_accounts.contains(address)) reserve->stop();
        session->accounts[address] = account;
        session->queues[address] = reserve;
    }
    if (session->accounts.empty()) throw std::runtime_error{"請先編輯空間並新增服務帳戶"};
    session->account = session->accounts.begin()->second;
    session->queue = session->queues.begin()->second;
    std::filesystem::create_directories(settings.root_path);
    if (std::filesystem::exists(session->root_path))
    {
        const auto root = nlohmann::json::parse(storage_read(session->root_path));
        session->root = root.at("id");
        session->root_account = root.at("account");
    }
    else
    {
        session->root_account = session->choose(1);
        google_easy_t request;
        request.m_account_context = session->accounts.at(session->root_account);
        request.control_set(control);
        session->root = folder_root_create(request);
        storage_atomic_write(session->root_path,nlohmann::json{{"account",session->root_account},{"id",session->root}}.dump());
    }
    session->remember(session->root,session->root_account,"folder");
    session->clearing = std::ranges::any_of(space.accounts,[&](const auto& address)
        { return clearing_accounts.contains(address); });
    std::lock_guard lock{m_mutex};
    std::erase_if(m_sessions,[&](const auto& active) { return active->storage_id==space.id; });
    m_sessions.push_back(session);
    return session;
}
auto storage_runtime_t::clear(const std::shared_ptr<storage_session_t>& session,
    const std::shared_ptr<request_control_t>& control, std::atomic_uint64_t& progress,
    std::atomic_uint64_t& total) -> std::string
{
    // Every linked account is cleared in full, including other spaces' blobs
    // and unlinked reserves. Freeze shared queues before taking the inventory.
    const auto marker=clear_path(session->storage_id);
    auto saved=std::filesystem::exists(marker) ? nlohmann::json::parse(storage_read(marker)) : nlohmann::json::object();
    if (saved.empty())
    {
        std::vector<std::string> addresses;
        for (const auto& [address, unused] : session->accounts) addresses.push_back(address);
        saved={{"accounts",addresses}};
        storage_atomic_write(marker,saved.dump());
    }
    for (const auto& [address, reserve] : session->queues) reserve->stop();
    auto ids=saved.value("ids",nlohmann::json::array());
    if (!saved.contains("ids"))
    {
        for (const auto& [address, account] : session->accounts)
        {
            google_easy_t request;
            request.m_account_context=account;
            request.control_set(control);
            std::string page;
            do
            {
                request.clear();
                request.path_set("/drive/v3/files");
                request.query_set("pageSize","1000");
                request.query_set("fields","nextPageToken,files(id,name)");
                if (!page.empty()) request.query_set("pageToken",page);
                request.method_get(); request.run();
                const auto response=nlohmann::json::parse(request.payload());
                for (const auto& file : response.at("files"))
                {
                    // Arbitrary Drive files may not use our JSON names. Only
                    // folder metadata needs a direct request; all other files
                    // remain eligible for same-account deletion batches.
                    const auto name=nlohmann::json::parse(file.value("name",std::string{}),nullptr,false);
                    const bool folder=name.is_object() && name.contains("next") && !name.contains("size");
                    ids.push_back({{"id",file.at("id")},{"account",address},{"kind",folder ? "folder" : "file"}});
                }
                page=response.value("nextPageToken",std::string{});
                total=ids.size();
            } while (!page.empty());
        }
        // Finish and persist every account's inventory before deleting anything;
        // paging an account while deleting its files can skip entries.
        saved["ids"]=ids;
        saved["completed"]=nlohmann::json::array();
        storage_atomic_write(marker,saved.dump());
    }
    for (const auto& [address,reserve] : session->queues)
    {
        reserve->clear_end();
        reserve->clear_begin();
    }
    std::set<std::string> completed=saved.at("completed").get<std::set<std::string>>();
    total=ids.size(); progress=completed.size();
    for (const auto& item : ids)
    {
        const auto first=item.at("id").get<std::string>();
        if (completed.contains(first)) continue;
        const auto address=item.at("account").get<std::string>();
        const bool single = item.at("kind")=="folder" || !batch_request.load();
        const bool folder=item.at("kind")=="folder";
        std::vector<std::string> group;
        for (const auto& other : ids)
            if (!completed.contains(other.at("id").get<std::string>()) && other.at("account")==address &&
                (other.at("kind")=="folder")==folder && group.size()<(single ? 1u : 100u))
                group.push_back(other.at("id"));
        if (single)
        {
            class delete_t : public google_easy_t
            { public: auto valid() -> bool override { return m_response_code==404 || google_easy_t::valid(); } };
            delete_t request;
            request.m_account_context=session->accounts.at(address);
            request.control_set(control);
            request.path_set("/drive/v3/files/"+group.front()); request.method_delete(); request.run();
        }
        else
        {
            google_batch_t batch;
            batch.boundary_set("+");
            batch.m_account_context=session->accounts.at(address);
            batch.control_set(control); batch.path_set("/drive/v3");
            for (std::size_t i{};i<group.size();++i)
            {
                auto& request=batch.request_add_handle();
                request.path_set("/drive/v3/files/"+group[i]); request.method_set("DELETE");
                batch.response_allow_missing(i);
            }
            batch.method_post(); batch.run();
        }
        completed.insert(group.begin(),group.end());
        progress=completed.size();
        saved["completed"]=completed;
        storage_atomic_write(marker,saved.dump());
    }
    // Commit the new root identity before removing the clear checkpoint. A lost
    // response may leave an unreachable empty root; retry uses the saved identity.
    auto address=saved.value("new_root_account",std::string{});
    if (address.empty()) { address=session->choose(1); saved["new_root_account"]=address; storage_atomic_write(marker,saved.dump()); }
    google_easy_t request;
    request.m_account_context=session->accounts.at(address); request.control_set(control);
    const auto root=saved.contains("new_root") ? saved.at("new_root").get<std::string>() : folder_root_create(request);
    saved["new_root"]=root; storage_atomic_write(marker,saved.dump());
    storage_atomic_write(session->root_path,nlohmann::json{{"account",address},{"id",root}}.dump());
    session->root_account=address;
    session->remember(root,address,"folder");
    std::filesystem::remove(marker);
    return root;
}
auto storage_runtime_t::cleared(const std::shared_ptr<storage_session_t>& session, std::string root) -> void
{
    std::lock_guard lock{m_mutex};
    // One queue per account, including when several spaces share it. Every
    // previous reserve ID was deleted, so replace all references together.
    for (auto& [address,reserve] : session->queues)
    {
        auto replacement=std::make_shared<blob_queue_t>(queue_path(address),session->accounts.at(address).get(),m_close);
        m_queues[address]=replacement;
        for (auto& context : m_sessions)
            if (context->queues.contains(address))
            {
                context->queues[address]=replacement;
                context->queue=context->queues.begin()->second;
                context->clearing=false;
            }
    }
    session->root=std::move(root);
    session->clearing=false;
}
auto storage_runtime_t::stop() -> void
{
    std::lock_guard lock{m_mutex};
    for (auto& [email, account] : m_accounts)
        account->stop();
    for (auto& [email, queue] : m_queues)
        queue->stop();
}

auto storage_runtime_t::sign_out(const std::string& storage_id) -> void
{
    std::lock_guard lock{m_mutex};
    std::erase_if(m_sessions,[&](const auto& session) { return session->storage_id==storage_id; });
    // Queues and credential workers are per account, possibly shared by another
    // signed-in space. Stop only identities no remaining space uses.
    std::erase_if(m_queues,[&](const auto& entry)
    {
        if (std::ranges::any_of(m_sessions,[&](const auto& session)
            { return session->accounts.contains(entry.first); })) return false;
        entry.second->stop();
        return true;
    });
    std::erase_if(m_accounts,[&](const auto& entry)
    {
        if (m_queues.contains(entry.first)) return false;
        entry.second->stop();
        return true;
    });
}

auto storage_runtime_t::retry_accounts() -> void
{
    std::lock_guard lock{m_mutex};
    for (auto& [email, account] : m_accounts)
        account->retry();
}

auto storage_runtime_t::signed_in_sessions() -> std::vector<std::shared_ptr<storage_session_t>>
{
    std::lock_guard lock{m_mutex};
    std::vector<std::shared_ptr<storage_session_t>> sessions;
    std::set<std::string> identities;
    for (const auto& session : m_sessions)
        if (identities.insert(session->storage_id).second) sessions.push_back(session);
    return sessions;
}
