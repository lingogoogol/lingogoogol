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
    std::error_code error;
    if (!std::filesystem::exists(directory, error) && !error) return accounts;
    auto entry = std::filesystem::directory_iterator{
        directory, std::filesystem::directory_options::skip_permission_denied, error};
    if (error) throw std::runtime_error{"Cannot read the service account directory."};
    const auto end = std::filesystem::directory_iterator{};
    for (; entry != end; entry.increment(error))
    {
        std::error_code file_error;
        if (!entry->is_regular_file(file_error)) continue;
        try
        {
            // The JSON contents identify an account; names and extensions do
            // not. One bad or unrelated file must not hide the other accounts.
            const auto credential = nlohmann::json::parse(storage_read(entry->path()));
            const auto address = credential.value("client_email", std::string{});
            if (address.empty() || credential.value("private_key", std::string{}).empty()) continue;
            accounts.push_back({address, entry->path().generic_string()});
        }
        catch (const std::exception&) { }
    }
    if (error) throw std::runtime_error{"Cannot enumerate the service account directory."};
    std::ranges::sort(accounts, [](const auto& a, const auto& b)
        { return std::tie(a.address,a.account_path) < std::tie(b.address,b.account_path); });
    return accounts;
}
storage_runtime_t::storage_runtime_t(std::atomic_bool* close) : m_close{close}
{
}
auto storage_runtime_t::queue(const std::shared_ptr<storage_session_t>& session)
    -> std::shared_ptr<blob_queue_t>
{
    auto& queue{m_queues[session->storage_id]};
    if (!queue)
    {
        queue = std::make_shared<blob_queue_t>(queue_path(session->storage_id), session->account.get(), m_close);
        if (std::filesystem::exists(clear_path(session->storage_id)))
            queue->clear_begin();
    }
    return queue;
}
auto storage_runtime_t::open(const storage_settings_t& settings,
                             const std::shared_ptr<request_control_t>& control, std::string expected_email,
                             std::string saved_root, std::string expected_storage) -> std::shared_ptr<storage_session_t>
{
    const auto credential = nlohmann::json::parse(storage_read(settings.account_path));
    if (credential.contains("installed"))
    {
        auto user = google_oauth_user_t::restore(settings.account_path,expected_email);
        if (!user)
        {
            if (!expected_email.empty()) throw std::runtime_error{"Sign in to restore this Google account task."};
            return {};
        }
        if (!expected_storage.empty() && expected_storage!=user->storage_id())
            throw std::runtime_error{"Saved task belongs to a different OAuth application data space."};
        return open_oauth(settings,std::move(user),control,std::move(saved_root));
    }
    const auto address{credential.at("client_email").get<std::string>()};
    if (!expected_email.empty() && expected_email != address)
        throw std::runtime_error{"Saved task account no longer matches its credential file."};
    auto session{std::make_shared<storage_session_t>()};
    session->clearing = std::filesystem::exists(clear_path(address));
    session->address = address;
    session->storage_id = address;
    session->account_path = settings.account_path;
    // Validate/create only the configured directory, before any cloud creation.
    std::filesystem::create_directories(settings.root_path);
    session->root_path = (std::filesystem::path{settings.root_path}/storage_digest(session->storage_id)).generic_string();
    {
        std::lock_guard lock{m_mutex};
        if (*m_close)
            throw std::runtime_error{"Application is closing."};
        auto& account{
            m_accounts[address + ":" + storage_digest(credential.at("private_key").get<std::string>())]};
        if (!account)
            account =
                std::make_shared<google_account_t>(address, credential.at("private_key").get<std::string>());
        session->account = account;
        session->queue = queue(session);
        m_sessions.push_back(session);
    }
    if (!saved_root.empty())
        session->root = std::move(saved_root);
    else if (std::filesystem::exists(session->root_path))
        session->root = storage_read(session->root_path);
    else
    {
        google_easy_t request;
        request.m_account_context = session->account;
        request.control_set(control);
        session->root = folder_root_create(request);
        storage_atomic_write(session->root_path, session->root);
    }
    if (session->root.empty())
        throw std::runtime_error{"Root file is empty."};
    return session;
}
auto storage_runtime_t::open_oauth(const storage_settings_t& settings,
    std::shared_ptr<google_oauth_user_t> user, const std::shared_ptr<request_control_t>& control,
    std::string saved_root) -> std::shared_ptr<storage_session_t>
{
    auto session = std::make_shared<storage_session_t>();
    session->oauth = user;
    session->address = user->address();
    session->storage_id = user->storage_id();
    session->account_path = settings.account_path;
    // Use the same account-specific file naming for service accounts and OAuth.
    // The OAuth storage ID includes its client, so two apps cannot share roots.
    std::filesystem::create_directories(settings.root_path);
    session->root_path = (std::filesystem::path{settings.root_path}/storage_digest(session->storage_id)).generic_string();
    session->clearing = std::filesystem::exists(clear_path(session->storage_id));
    {
        std::lock_guard lock{m_mutex};
        if (*m_close) throw std::runtime_error{"Application is closing."};
        auto& account = m_accounts[session->storage_id];
        if (!account)
        {
            account = std::make_shared<google_account_t>();
            account->init_oauth(user);
        }
        else account->update_oauth(user);
        // Same-user queues and restored tasks retain this stable account object
        // and immediately gain new credentials after re-login. Other identities
        // remain isolated in their original account objects and queues.
        session->account = account;
        session->queue = queue(session);
        m_sessions.push_back(session);
    }
    if (!saved_root.empty()) session->root = std::move(saved_root);
    else if (std::filesystem::exists(session->root_path)) session->root = storage_read(session->root_path);
    else
    {
        google_easy_t request;
        request.m_account_context = session->account;
        request.control_set(control);
        session->root = folder_root_create(request);
        storage_atomic_write(session->root_path,session->root);
    }
    if (session->root.empty()) throw std::runtime_error{"Root file is empty."};
    return session;
}

auto storage_runtime_t::clear(const std::shared_ptr<storage_session_t>& session,
                              const std::shared_ptr<request_control_t>& control,
                              std::atomic_uint64_t& progress, std::atomic_uint64_t& total) -> std::string
{
    // Stop replenishment and wait for its requests before collecting the full
    // account inventory. Deleting while paging would skip entries.
    session->queue->stop();
    google_easy_t request;
    request.m_account_context = session->account;
    request.control_set(control);
    const auto marker{clear_path(session->storage_id)};
    std::vector<std::string> ids;
    if (std::filesystem::exists(marker))
    {
        const auto saved = nlohmann::json::parse(storage_read(marker));
        ids = saved.at("ids").get<std::vector<std::string>>();
        progress = saved.at("progress").get<std::uint64_t>();
        total = ids.size();
    }
    else
    {
        std::string page;
        do
        {
            request.clear();
            request.path_set("/drive/v3/files");
            request.query_set("pageSize", "1000");
            if (session->oauth) request.query_set("spaces","appDataFolder");
            request.query_set("fields", "nextPageToken,files(id)");
            if (!page.empty())
                request.query_set("pageToken", page);
            request.method_get();
            request.run();
            const auto response = nlohmann::json::parse(request.payload());
            for (const auto& file : response.at("files"))
                ids.push_back(file.at("id").get<std::string>());
            page = response.value("nextPageToken", "");
            total = ids.size();
        } while (!page.empty());
        storage_atomic_write(marker, nlohmann::json{{"ids", ids}, {"progress", 0}}.dump());
    }
    // Empty the durable reserve before the first DELETE, so a crash cannot
    // restore IDs belonging to files that have already been removed.
    session->queue->clear_end();
    session->queue->clear_begin();
    google_batch_t remove;
    remove.m_account_context = session->account;
    remove.control_set(control);
    remove.boundary_set("+");
    for (std::size_t index{progress.load()}; index < ids.size();)
    {
        remove.clear();
        remove.path_set("/drive/v3");
        const auto count{std::min<std::size_t>(google_batch_t::part_count_max(), ids.size()-index)};
        for (std::size_t part{}; part<count; ++part)
        {
            auto& request{remove.request_add_handle()};
            request.path_set("/drive/v3/files/" + ids[index+part]);
            request.method_set("DELETE");
            remove.response_allow_missing(part);
        }
        remove.method_post();
        remove.run();
        // Advance only after every part succeeds. A lost response/crash replays
        // this batch; 404 is accepted only for these idempotent deletes.
        index += count;
        progress = index;
        storage_atomic_write(marker, nlohmann::json{{"ids", ids}, {"progress", progress.load()}}.dump());
    }
    // The app needs a fresh empty root after literally all Drive files are removed.
    auto root{folder_root_create(request)};
    storage_atomic_write(session->root_path, root);
    std::filesystem::remove(marker);
    return root;
}
auto storage_runtime_t::cleared(const std::shared_ptr<storage_session_t>& session, std::string root) -> void
{
    std::lock_guard lock{m_mutex};
    m_queues.erase(session->storage_id);
    auto replacement{queue(session)};
    for (auto& context : m_sessions)
        if (context->storage_id == session->storage_id)
        {
            context->queue = replacement;
            context->clearing = false;
            if (context->root_path == session->root_path)
                context->root = root;
            else
            {
                context->root = root;
                storage_atomic_write(context->root_path, root);
            }
        }
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
    if (const auto queue = m_queues.find(storage_id); queue!=m_queues.end())
    {
        queue->second->stop();
        m_queues.erase(queue);
    }
    std::erase_if(m_accounts,[&](const auto& entry)
    {
        const bool matches = std::ranges::any_of(m_sessions,[&](const auto& session)
            { return session->storage_id==storage_id && session->account==entry.second; });
        if (matches) entry.second->stop();
        return matches;
    });
    std::erase_if(m_sessions,[&](const auto& session) { return session->storage_id==storage_id; });
}

auto storage_runtime_t::retry_accounts() -> void
{
    std::lock_guard lock{m_mutex};
    for (auto& [email, account] : m_accounts)
        account->retry();
}
