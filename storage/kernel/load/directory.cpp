module;
#include <nlohmann/json.hpp>
module storage.kernel.load;
import std;
import storage.google;
import storage.http;

namespace
{
    using response_t = std::future<std::string>;

    // Handle-independent descriptions let the background coordinator await I/O
    // without creating another easy handle or touching the GUI-owned scheduler.
    class directory_io_t
    {
        directory_task_t& task;
        auto submit(std::function<void(google_easy_t&)> configure, bool allow_missing = false) -> response_t
        {
            if (task.m_control->cancelled) throw std::runtime_error{"操作已中止"};
            auto request{std::make_shared<directory_request_t>()};
            request->configure = std::move(configure);
            request->allow_missing = allow_missing;
            auto result{request->response.get_future()};
            std::unique_lock lock{task.m_requests_mutex};
            task.m_requests.push_back(std::move(request));
            return result;
        }
      public:
        explicit directory_io_t(directory_task_t& owner) : task{owner} {}
        auto wait(response_t result) -> std::string
        {
            using namespace std::chrono_literals;
            while (result.wait_for(10ms) != std::future_status::ready)
                if (task.m_control->cancelled) throw std::runtime_error{"操作已中止"};
            if (task.m_control->cancelled) throw std::runtime_error{"操作已中止"};
            return result.get();
        }
        auto get_async(std::string id, std::string fields) -> response_t
        {
            return submit([id = std::move(id), fields = std::move(fields)](auto& easy)
                { blob_get(easy,id,fields); });
        }
        auto get(std::string id, std::string fields) -> nlohmann::json
        { return nlohmann::json::parse(wait(get_async(std::move(id),std::move(fields)))); }
        auto get_name(std::string id) -> std::string
        { return get(std::move(id),"name").at("name").get<std::string>(); }
        auto update_async(blob_str_opt_t blob) -> response_t
        { return submit([blob = std::move(blob)](auto& easy) { blob_update(easy,blob); }); }
        auto update(blob_str_opt_t blob) -> void { wait(update_async(std::move(blob))); }
        auto update_name(std::string id, std::string name) -> void
        { update({std::move(id),std::move(name),std::nullopt}); }
        auto update_content(std::string id, std::string content) -> void
        { update({std::move(id),std::nullopt,std::move(content)}); }
        auto remove(std::string id) -> response_t
        {
            return submit([id = std::move(id)](auto& easy)
                { easy.clear(); easy.path_set("/drive/v3/files/"+id); easy.method_delete(); },true);
        }
    };

    // At most one pool request per logical job, including a data GET followed
    // by PATCH. Consume whichever finishes first, not just the first submitted.
    // Discovery can grow size() as new metadata links arrive.
    // Complete may return a follow-up future for the same job (a data PATCH
    // after its GET), or an invalid future once that job has fully succeeded.
    template<typename Size, typename Start, typename Complete>
    auto parallel(directory_task_t& task, std::size_t begin, Size size, Start start, Complete complete) -> void
    {
        using namespace std::chrono_literals;
        std::map<std::size_t,response_t> pending;
        auto next{begin};
        while (next<size() || !pending.empty())
        {
            if (task.m_control->cancelled) throw std::runtime_error{"操作已中止"};
            while (pending.size()<transfer_slot_count && next<size())
            { pending.emplace(next,start(next)); ++next; }
            bool progressed{};
            for (auto entry{pending.begin()}; entry!=pending.end();)
            {
                if (entry->second.wait_for(0ms)!=std::future_status::ready) { ++entry; continue; }
                auto follow{complete(entry->first,entry->second.get())};
                progressed=true;
                if (follow.valid()) { entry->second=std::move(follow); ++entry; }
                else entry=pending.erase(entry);
            }
            if (!progressed) std::this_thread::sleep_for(1ms);
        }
    }

    class directory_easy_t : public google_easy_t
    {
      public:
        explicit directory_easy_t(curl_easy_t&& easy) : google_easy_t{std::move(easy)} {}
        auto valid() -> bool override
        {
            // Surface missing items on the coordinator, not as a network retry
            // or an exception that could stop the shared transfer worker.
            return m_response_code==404 || google_easy_t::valid();
        }
    };

    struct page_t
    {
        std::string id;
        nlohmann::json name, content;
    };

    auto pages(directory_io_t& easy, std::string id) -> std::vector<page_t>
    {
        std::vector<page_t> result;
        std::set<std::string> seen;
        while (id != "null")
        {
            if (id.empty() || !seen.insert(id).second)
                throw std::runtime_error{"目錄或檔案索引包含循環"};
            const auto blob = easy.get(id, "name,description");
            auto name = nlohmann::json::parse(blob.at("name").get<std::string>());
            auto content = nlohmann::json::parse(blob.at("description").get<std::string>());
            if (!content.is_array())
                throw std::runtime_error{"索引格式不正確"};
            result.push_back({id, name, std::move(content)});
            id = name.at("next").get<std::string>();
        }
        return result;
    }

    auto check_destination(directory_io_t& easy, const storage_session_t& session, std::string id,
                           const std::string& moving = {}) -> void
    {
        std::set<std::string> seen;
        while (true)
        {
            if (id.empty() || id == "null" || !seen.insert(id).second)
                throw std::runtime_error{"目的地不在目前根目錄內"};
            if (id == moving)
                throw std::runtime_error{"不能移到自己或自己的子資料夾"};
            if (id == session.root)
                return;
            const auto name = nlohmann::json::parse(easy.get_name(id));
            if (name.contains("size"))
                throw std::runtime_error{"目的地必須是資料夾"};
            id = name.at("parent").get<std::string>();
        }
    }

    auto reserve(directory_task_t& task, const std::string& key) -> std::string
    {
        return task.m_journal
            ->allocate(key, 1,
                       [&](std::size_t count) {
                           return task.m_session->queue->get(count, &task.m_control->cancelled);
                       })
            .front();
    }

    auto insert(directory_io_t& easy, directory_task_t& task, const std::string& parent,
                const nlohmann::json& entry) -> void
    {
        if (nlohmann::json::array({entry}).dump().size() > content_size_max)
            throw std::runtime_error{"項目名稱過長"};
        auto chain{pages(easy, parent)};
        for (const auto& page : chain)
            for (const auto& other : page.content)
            {
                if (other.at("id") == entry.at("id"))
                    return; // Idempotent after a lost response/crash.
            }
        auto& last{chain.back()};
        last.content.push_back(entry);
        if (last.content.size() <= 100 && last.content.dump().size() <= content_size_max)
            easy.update_content(last.id, last.content.dump());
        else
        {
            const auto id{reserve(task, "directory-page:" + parent)};
            auto name = last.name;
            name["next"] = "null";
            easy.update(blob_str_opt_t{id, name.dump(), nlohmann::json::array({entry}).dump()});
            last.name["next"] = id;
            easy.update_name(last.id, last.name.dump());
        }
    }

    auto unlink(directory_io_t& easy, const std::string& parent, const std::string& id) -> void
    {
        for (auto& page : pages(easy, parent))
        {
            const auto size{page.content.size()};
            std::erase_if(page.content.get_ref<nlohmann::json::array_t&>(),
                          [&](const auto& entry) { return entry.at("id") == id; });
            if (page.content.size() != size)
                easy.update_content(page.id, page.content.dump());
        }
    }

    auto verify_entry(directory_io_t& easy, const std::string& parent, const nlohmann::json& entry) -> void
    {
        for (const auto& page : pages(easy, parent))
            for (const auto& other : page.content)
                if (other.at("id") == entry.at("id") && other.at("type") == entry.at("type"))
                    return;
        throw std::runtime_error{"項目已不在原資料夾，請重新整理"};
    }

    // Inspect the whole graph before unlinking/deleting anything. Validate
    // parent IDs so malformed metadata cannot delete unrelated cloud blobs.
    auto collect(directory_task_t& task, directory_io_t& easy, const nlohmann::json& entry,
                 const std::string& parent, std::set<std::string>& seen,
                 std::vector<std::string>& ids, nlohmann::json* snapshot = nullptr) -> void
    {
        struct node_t { std::string id, kind, parent, root; };
        std::vector<node_t> nodes;
        auto add = [&](std::string id, std::string kind, std::string owner, std::string root)
        {
            if (id.empty() || id=="null" || !seen.insert(id).second)
                throw std::runtime_error{"索引重複或包含循環"};
            nodes.push_back({std::move(id),std::move(kind),std::move(owner),std::move(root)});
        };
        const auto root{entry.at("id").get<std::string>()};
        add(root,entry.at("type").get<std::string>(),parent,root);
        parallel(task,0,[&] { return nodes.size(); },
            [&](std::size_t i) { return easy.get_async(nodes[i].id,nodes[i].kind=="data" ? "name" : "name,description"); },
            [&](std::size_t i, std::string response) -> response_t
            {
                // Copy before appending: discovery can reallocate nodes.
                const auto node{nodes[i]};
                const auto blob=nlohmann::json::parse(response);
                const auto name=nlohmann::json::parse(blob.at("name").get<std::string>());
                if (name.at("parent")!=node.parent)
                    throw std::runtime_error{"索引的父資料夾不一致"};
                ids.push_back(node.id);
                if (node.kind=="data")
                {
                    if (snapshot) snapshot->push_back({{"id",node.id},{"kind",node.kind},{"name",name}});
                    return {};
                }
                if (node.kind!="folder" && node.kind!="file" && node.kind!="chunk")
                    throw std::runtime_error{"未知項目類型"};
                const auto content=nlohmann::json::parse(blob.at("description").get<std::string>());
                if (!content.is_array()) throw std::runtime_error{"索引格式不正確"};
                if (snapshot) snapshot->push_back({{"id",node.id},{"kind",node.kind},{"name",name},{"content",content}});
                if (node.kind!="chunk" && name.at("next")!="null")
                    add(name.at("next").get<std::string>(),node.kind,node.parent,node.root);
                for (const auto& child : content)
                {
                    if (node.kind=="folder")
                    {
                        const auto id{child.at("id").get<std::string>()};
                        add(id,child.at("type").get<std::string>(),node.root,id);
                    }
                    else if (node.kind=="file")
                        add(child.get<std::string>(),"chunk",node.root,{});
                    else
                        add(child.get<std::string>(),"data",node.id,{});
                }
                return {};
            });
    }

} // namespace

auto directory_validate_name(std::string_view name) -> void
{
    if (name.empty() || name == "." || name == ".." ||
        std::ranges::all_of(name, [](unsigned char c) { return c <= ' '; }))
        throw std::runtime_error{"請輸入資料夾名稱"};
    if (std::ranges::any_of(name, [](unsigned char c) { return c < 32 || c == '/' || c == '\\'; }))
        throw std::runtime_error{"名稱不可包含斜線或控制字元"};
}

auto directory_execute(directory_task_t& task) -> void
{
    directory_io_t easy{task};
    // Do not cache mutable directory GETs: retry/restore must read the latest
    // pages. Explicit journal phases and stable IDs make writes replayable.
    auto saved = task.m_journal->data();
    check_destination(easy, *task.m_session, task.m_source);
    if (task.m_kind == "directory-create")
    {
        directory_validate_name(task.m_name);
        task.m_total = 2;
        const auto id{reserve(task, "created-directory")};
        if (!saved.value("initialized", false))
        {
            const auto name =
                nlohmann::json{{"name", task.m_name}, {"parent", task.m_source}, {"next", "null"}};
            easy.update({id, name.dump(), "[]"});
            task.m_journal->set("initialized", true);
        }
        task.m_completed = 1;
        insert(easy, task, task.m_source,
               nlohmann::json{{"type", "folder"}, {"id", id}, {"name", task.m_name}});
        task.m_completed = 2;
        return;
    }
    const auto id{task.m_entity.at("id").get<std::string>()};
    if (id == task.m_session->root)
        throw std::runtime_error{"不可操作根目錄"};
    if (task.m_kind == "directory-move")
    {
        task.m_total = 3;
        check_destination(easy, *task.m_session, task.m_destination,
                          task.m_entity.at("type") == "folder" ? id : std::string{});
        if (task.m_destination == task.m_source)
        {
            task.m_completed = 3;
            return;
        }
        if (!saved.value("prepared", false))
        {
            verify_entry(easy, task.m_source, task.m_entity);
            task.m_journal->set("prepared", true);
        }
        auto moved_pages{pages(easy, id)};
        for (const auto& page : moved_pages)
        {
            if ((page.name.at("parent") != task.m_source && page.name.at("parent") != task.m_destination) ||
                page.name.contains("size") != (task.m_entity.at("type") == "file"))
                throw std::runtime_error{"項目索引與來源資料夾不一致"};
        }
        // Publish the destination first. A crash can temporarily leave two
        // links, but never loses the only link; restoration finishes unlinking.
        insert(easy, task, task.m_destination, task.m_entity);
        task.m_completed = 1;
        for (auto& page : moved_pages)
        {
            page.name["parent"] = task.m_destination;
            easy.update_name(page.id, page.name.dump());
        }
        task.m_completed = 2;
        unlink(easy, task.m_source, id);
        task.m_completed = 3;
        return;
    }
    if (task.m_kind == "directory-copy")
    {
        check_destination(easy, *task.m_session, task.m_destination,
                          task.m_entity.at("type") == "folder" ? id : std::string{});
        nlohmann::json snapshot;
        if (const auto stored{task.m_journal->response("directory-copy-manifest")})
            snapshot = nlohmann::json::parse(*stored);
        else
        {
            verify_entry(easy, task.m_source, task.m_entity);
            snapshot = nlohmann::json::array();
            std::set<std::string> seen;
            std::vector<std::string> ids;
            collect(task, easy, task.m_entity, task.m_source, seen, ids, &snapshot);
            // Only metadata goes in the manifest. Immutable data payloads are
            // streamed through the bounded window instead of loading the file.
            task.m_journal->response("directory-copy-manifest", snapshot.dump());
        }
        std::vector<std::string> reserved;
        reserved.reserve(snapshot.size());
        // The queue replenishes only to 20,000 IDs. A reservation larger than
        // that limit can never complete; journal bounded batches before use.
        for (std::size_t offset{}; offset < snapshot.size(); offset += 100)
        {
            const auto batch{task.m_journal->allocate("copied-blobs:" + std::to_string(offset),
                std::min<std::size_t>(100, snapshot.size() - offset), [&](std::size_t count) {
                    return task.m_session->queue->get(count, &task.m_control->cancelled);
                })};
            reserved.insert(reserved.end(), batch.begin(), batch.end());
        }
        std::map<std::string, std::string> copies;
        for (std::size_t i{}; i < snapshot.size(); ++i)
            copies.emplace(snapshot.at(i).at("id").get<std::string>(), reserved.at(i));
        task.m_total = snapshot.size() + 1;
        const auto begin{saved.value("copy_index", std::size_t{})};
        task.m_completed = begin;
        if (begin>snapshot.size()) throw std::runtime_error{"複製進度超出索引範圍"};
        std::vector<bool> writing(snapshot.size()), completed(snapshot.size());
        auto prefix{begin}, checkpoint{begin};
        const auto clone = [&](std::size_t i, std::string content) -> response_t
        {
            const auto& node{snapshot.at(i)};
            auto name=node.at("name");
            const auto parent{name.at("parent").get<std::string>()};
            name["parent"]=copies.contains(parent) ? copies.at(parent) : task.m_destination;
            if (name.contains("next") && name.at("next")!="null")
                name["next"]=copies.at(name.at("next").get<std::string>());
            return easy.update_async({copies.at(node.at("id").get<std::string>()),name.dump(),std::move(content)});
        };
        parallel(task,begin,[&] { return snapshot.size(); },
            [&](std::size_t i) -> response_t
            {
                const auto& node{snapshot.at(i)};
                if (node.at("kind")=="data")
                    return easy.get_async(node.at("id").get<std::string>(),"description");
                auto links=node.at("content");
                for (auto& link : links)
                    if (node.at("kind")=="folder")
                        link["id"]=copies.at(link.at("id").get<std::string>());
                    else link=copies.at(link.get<std::string>());
                writing[i]=true;
                return clone(i,links.dump());
            },
            [&](std::size_t i, std::string response) -> response_t
            {
                if (!writing[i])
                {
                    writing[i]=true;
                    return clone(i,nlohmann::json::parse(response).at("description").get<std::string>());
                }
                completed[i]=true;
                ++task.m_completed;
                // Responses arrive out of order. Persist only the contiguous
                // successful prefix; later writes replay against stable IDs.
                while (prefix<snapshot.size() && completed[prefix]) ++prefix;
                if (prefix/100!=checkpoint/100 || prefix==snapshot.size())
                { task.m_journal->set("copy_index",prefix); checkpoint=prefix; }
                return {};
            });
        auto entry = task.m_entity;
        entry["id"] = copies.at(id);
        // Publish only after every cloned page/chunk/data blob has succeeded.
        // Stable reserved IDs make an interrupted PATCH or publication replayable.
        insert(easy, task, task.m_destination, entry);
        task.m_completed = task.m_total.load();
        return;
    }
    if (task.m_kind != "directory-delete")
        throw std::runtime_error{"未知目錄操作"};
    std::vector<std::string> ids;
    if (saved.contains("delete_ids"))
        ids = saved.at("delete_ids").get<std::vector<std::string>>();
    else
    {
        verify_entry(easy, task.m_source, task.m_entity);
        std::set<std::string> seen;
        collect(task, easy, task.m_entity, task.m_source, seen, ids);
        task.m_journal->set("delete_ids", ids); // Durable before unlinking the only visible entry.
    }
    unlink(easy, task.m_source, id);
    task.m_total = ids.size();
    auto begin{saved.value("delete_index", std::size_t{})};
    task.m_completed = begin;
    if (begin>ids.size()) throw std::runtime_error{"刪除進度超出索引範圍"};
    std::vector<bool> completed(ids.size());
    auto prefix{begin}, checkpoint{begin};
    parallel(task,begin,[&] { return ids.size(); },
        [&](std::size_t i) { return easy.remove(ids[i]); },
        [&](std::size_t i, std::string) -> response_t
        {
            completed[i]=true;
            ++task.m_completed;
            while (prefix<ids.size() && completed[prefix]) ++prefix;
            if (prefix/100!=checkpoint/100 || prefix==ids.size())
            { task.m_journal->set("delete_index",prefix); checkpoint=prefix; }
            return {};
        });
}

auto directory_transfer_begin(directory_task_t& task, curl_multi_worker_t& multi,
                              curl_easy_t&& easy) -> bool
{
    std::shared_ptr<directory_request_t> request;
    {
        std::unique_lock lock{task.m_requests_mutex};
        if (task.m_requests.empty()) return false;
        request=std::move(task.m_requests.front());
        task.m_requests.pop_front();
    }
    directory_easy_t operation{std::move(easy)};
    operation.m_journal.reset(); // Mutable directory reads must never be replayed from a cache.
    auto* key{operation.interface_get()};
    try
    {
        request->configure(operation);
        task.m_state->m_end.emplace(key,[request](std::unique_ptr<curl_easy_t>& result)
        {
            const auto code{curl_easy_multi_access_t::response_code_get(*result)};
            if (code==404 && !request->allow_missing)
                request->response.set_exception(std::make_exception_ptr(
                    std::runtime_error{"項目或資料夾已不存在，請重新整理"}));
            else request->response.set_value(result->payload());
        });
    }
    catch (...)
    {
        task.m_state->m_end.erase(key);
        request->response.set_exception(std::current_exception());
        // Restore the pool slot if request construction failed.
        easy=std::move(operation);
        return false;
    }
    // Submission failures are worker failures, as for upload/download. Let the
    // caller unwind rather than returning an already-transferred empty slot.
    multi.add(std::move(operation));
    return true;
}
