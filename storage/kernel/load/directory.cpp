module;
#include <nlohmann/json.hpp>
module storage.kernel.load;
import std;
import storage.google;
import storage.http;

namespace
{
    // A queued operation may outlive its source/destination. A Drive 404 is
    // permanent here; treating it as a network outage would wait forever.
    class directory_easy_t : public google_easy_t
    {
        auto valid() -> bool override
        {
            if (m_response_code == 404)
                throw std::runtime_error{"項目或資料夾已不存在，請重新整理"};
            return google_easy_t::valid();
        }
    };

    struct page_t
    {
        std::string id;
        nlohmann::json name, content;
    };

    auto pages(google_easy_t& easy, std::string id) -> std::vector<page_t>
    {
        std::vector<page_t> result;
        std::set<std::string> seen;
        while (id != "null")
        {
            if (id.empty() || !seen.insert(id).second)
                throw std::runtime_error{"目錄或檔案索引包含循環"};
            const auto blob = blob_get_run(easy, id, "name,description");
            auto name = nlohmann::json::parse(blob.at("name").get<std::string>());
            auto content = nlohmann::json::parse(blob.at("description").get<std::string>());
            if (!content.is_array())
                throw std::runtime_error{"索引格式不正確"};
            result.push_back({id, name, std::move(content)});
            id = name.at("next").get<std::string>();
        }
        return result;
    }

    auto check_destination(google_easy_t& easy, const storage_session_t& session, std::string id,
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
            const auto name = nlohmann::json::parse(blob_get_name_run(easy, id));
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

    auto insert(google_easy_t& easy, directory_task_t& task, const std::string& parent,
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
            blob_update_content_run(easy, last.id, last.content.dump());
        else
        {
            const auto id{reserve(task, "directory-page:" + parent)};
            auto name = last.name;
            name["next"] = "null";
            blob_update_run(easy, blob_str_opt_t{id, name.dump(), nlohmann::json::array({entry}).dump()});
            last.name["next"] = id;
            blob_update_name_run(easy, last.id, last.name.dump());
        }
    }

    auto unlink(google_easy_t& easy, const std::string& parent, const std::string& id) -> void
    {
        for (auto& page : pages(easy, parent))
        {
            const auto size{page.content.size()};
            std::erase_if(page.content.get_ref<nlohmann::json::array_t&>(),
                          [&](const auto& entry) { return entry.at("id") == id; });
            if (page.content.size() != size)
                blob_update_content_run(easy, page.id, page.content.dump());
        }
    }

    auto verify_entry(google_easy_t& easy, const std::string& parent, const nlohmann::json& entry) -> void
    {
        for (const auto& page : pages(easy, parent))
            for (const auto& other : page.content)
                if (other.at("id") == entry.at("id") && other.at("type") == entry.at("type"))
                    return;
        throw std::runtime_error{"項目已不在原資料夾，請重新整理"};
    }

    // Inspect the whole graph before unlinking/deleting anything. Validate
    // parent IDs so malformed metadata cannot delete unrelated cloud blobs.
    auto collect(google_easy_t& easy, const nlohmann::json& entry, const std::string& parent,
                 std::set<std::string>& seen, std::vector<std::string>& ids,
                 nlohmann::json* snapshot = nullptr) -> void
    {
        const auto root{entry.at("id").get<std::string>()};
        const auto kind{entry.at("type").get<std::string>()};
        if (kind != "file" && kind != "folder")
            throw std::runtime_error{"未知項目類型"};
        const auto add = [&](const std::string& id) {
            if (!seen.insert(id).second)
                throw std::runtime_error{"索引重複或包含循環"};
            ids.push_back(id);
        };
        for (const auto& page : pages(easy, root))
        {
            if (page.name.at("parent") != parent)
                throw std::runtime_error{"索引的父資料夾不一致"};
            add(page.id);
            if (snapshot)
                snapshot->push_back(
                    {{"id", page.id}, {"kind", kind}, {"name", page.name}, {"content", page.content}});
            for (const auto& child : page.content)
            {
                if (kind == "folder")
                    collect(easy, child, root, seen, ids, snapshot);
                else
                {
                    const auto chunk{child.get<std::string>()};
                    const auto blob = blob_get_run(easy, chunk, "name,description");
                    const auto name = nlohmann::json::parse(blob.at("name").get<std::string>());
                    if (name.at("parent") != root)
                        throw std::runtime_error{"分塊不屬於此檔案"};
                    add(chunk);
                    const auto content = nlohmann::json::parse(blob.at("description").get<std::string>());
                    if (!content.is_array())
                        throw std::runtime_error{"內容索引格式不正確"};
                    if (snapshot)
                        snapshot->push_back(
                            {{"id", chunk}, {"kind", "chunk"}, {"name", name}, {"content", content}});
                    for (const auto& value : content)
                    {
                        const auto data{value.get<std::string>()};
                        const auto data_name = nlohmann::json::parse(blob_get_name_run(easy, data));
                        if (data_name.at("parent") != chunk)
                            throw std::runtime_error{"內容不屬於此分塊"};
                        add(data);
                        if (snapshot)
                            snapshot->push_back({{"id", data}, {"kind", "data"}, {"name", data_name}});
                    }
                }
            }
        }
    }

    class delete_easy_t : public google_easy_t
    {
        auto valid() -> bool override
        {
            return google_easy_t::valid() || m_response_code == 404;
        }
    };
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
    directory_easy_t easy;
    easy.m_account_context = task.m_session->account;
    easy.control_set(task.m_control);
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
            blob_update_run(easy, {id, name.dump(), "[]"});
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
            blob_update_name_run(easy, page.id, page.name.dump());
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
            collect(easy, task.m_entity, task.m_source, seen, ids, &snapshot);
            // Only metadata goes in the manifest. Data blobs are immutable and
            // streamed individually; only one data payload is resident at a time.
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
        for (auto i{begin}; i < snapshot.size(); ++i)
        {
            const auto& node{snapshot.at(i)};
            const auto original{node.at("id").get<std::string>()};
            auto name = node.at("name");
            const auto parent{name.at("parent").get<std::string>()};
            name["parent"] = copies.contains(parent) ? copies.at(parent) : task.m_destination;
            if (name.contains("next") && name.at("next") != "null")
                name["next"] = copies.at(name.at("next").get<std::string>());
            std::string content;
            if (node.at("kind") == "data")
                content = blob_get_content_run(easy, original);
            else
            {
                auto links = node.at("content");
                for (auto& link : links)
                    if (node.at("kind") == "folder")
                        link["id"] = copies.at(link.at("id").get<std::string>());
                    else
                        link = copies.at(link.get<std::string>());
                content = links.dump();
            }
            blob_update_run(easy, {copies.at(original), name.dump(), content});
            task.m_completed = i + 1;
            if ((i + 1) % 100 == 0 || i + 1 == snapshot.size())
                task.m_journal->set("copy_index", i + 1);
        }
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
        collect(easy, task.m_entity, task.m_source, seen, ids);
        task.m_journal->set("delete_ids", ids); // Durable before unlinking the only visible entry.
    }
    unlink(easy, task.m_source, id);
    task.m_total = ids.size();
    auto begin{saved.value("delete_index", std::size_t{})};
    task.m_completed = begin;
    delete_easy_t remove;
    remove.m_account_context = task.m_session->account;
    remove.control_set(task.m_control);
    for (auto i{begin}; i < ids.size(); ++i)
    {
        remove.clear();
        remove.path_set("/drive/v3/files/" + ids[i]);
        remove.method_delete();
        remove.run();
        task.m_completed = i + 1;
        // Reissuing DELETE after a crash is harmless (404 is success). Avoid
        // rewriting a potentially large ID manifest for every single blob.
        if ((i + 1) % 100 == 0 || i + 1 == ids.size())
            task.m_journal->set("delete_index", i + 1);
    }
}
