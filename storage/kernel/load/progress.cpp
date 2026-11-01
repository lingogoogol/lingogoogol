module;
#include <nlohmann/json.hpp>
module storage.kernel.load;
import std;

namespace
{
    using nlohmann::json;
    auto pages_save(const std::vector<upload_page_t>& pages) -> json
    {
        auto result=json::array();
        for (const auto& p:pages)
            result.push_back({{"id",p.id},{"next",p.next},{"begin",p.begin},{"count",p.count},{"complete",p.complete}});
        return result;
    }
    auto pages_restore(const json& saved) -> std::vector<upload_page_t>
    {
        std::vector<upload_page_t> result;
        for (const auto& p:saved)
            result.push_back({p.at("id"),p.at("next"),p.at("begin"),p.at("count"),p.at("complete"),false});
        return result;
    }
    auto prefix(const auto& entries, auto complete) -> std::size_t
    {
        std::size_t result{};
        while (result<entries.size() && complete(entries[result])) ++result;
        return result;
    }
}

auto upload_finished(const upload_entity_t& entity) -> bool
{
    if (const auto* file=dynamic_cast<const upload_file_t*>(&entity))
        return file->m_load_end && file->m_chunk_cover_progress==file->m_size &&
            std::ranges::all_of(file->m_chunk_arr,[](const auto& c) { return c.m_transfer_end && c.m_load_end && c.m_link_end; });
    const auto& folder=dynamic_cast<const upload_folder_t&>(entity);
    return folder.m_load_end && folder.m_folder_load_end_progress==folder.m_entity_arr.size();
}
auto upload_progress_save(upload_entity_t& entity) -> void
{
    if (!entity.m_task->m_journal) return;
    auto saved=json{{"main_id",dynamic_cast<upload_file_t*>(&entity) ?
        static_cast<upload_file_t&>(entity).m_main_id : static_cast<upload_folder_t&>(entity).m_main_id}};
    if (const auto* file=dynamic_cast<const upload_file_t*>(&entity))
    {
        saved["next_id"]=file->m_next_id;
        saved["chunk_cover_progress"]=file->m_chunk_cover_progress;
        saved["file_load_begin_progress"]=file->m_file_load_begin_progress;
        saved["file_load_end_progress"]=file->m_file_load_end_progress;
        saved["chunk_load_begin_progress"]=file->m_chunk_load_begin_progress;
        saved["chunk_load_end_progress"]=file->m_chunk_load_end_progress;
        saved["chunk_transfer_end_progress"]=file->m_chunk_transfer_end_progress;
        saved["load_end"]=file->m_load_end;
        saved["pages"]=pages_save(file->m_pages);
        saved["chunks"]=json::array();
        for (const auto& c:file->m_chunk_arr)
            saved["chunks"].push_back({{"id",c.m_id},{"content_id",c.m_content_id},{"size",c.m_size},
                {"transfer_end",c.m_transfer_end},{"load_end",c.m_load_end},{"link_end",c.m_link_end}});
    }
    else
    {
        const auto& folder=static_cast<const upload_folder_t&>(entity);
        saved["next_id"]=folder.m_next_id;
        saved["folder_load_begin_progress"]=folder.m_folder_load_begin_progress;
        saved["folder_load_end_progress"]=folder.m_folder_load_end_progress;
        saved["entity_load_end"]=folder.m_entity_load_end;
        saved["load_end"]=folder.m_load_end;
        saved["pages"]=pages_save(folder.m_pages);
    }
    entity.m_task->m_journal->progress("upload:"+entity.m_path_local,saved);
}
auto upload_progress_restore(upload_entity_t& entity) -> void
{
    if (!entity.m_task->m_journal) return;
    const auto stored=entity.m_task->m_journal->progress("upload:"+entity.m_path_local);
    if (!stored) return;
    const auto& s=*stored;
    if (auto* file=dynamic_cast<upload_file_t*>(&entity))
    {
        if (s.at("main_id")!=file->m_main_id) throw std::runtime_error{"Saved upload identity changed."};
        file->m_next_id=s.at("next_id");
        file->m_chunk_cover_progress=s.at("chunk_cover_progress");
        file->m_file_load_begin_progress=s.at("file_load_begin_progress");
        file->m_load_end=s.at("load_end");
        file->m_pages=pages_restore(s.at("pages"));
        for (const auto& c:s.at("chunks"))
            file->m_chunk_arr.push_back({.m_file=file,.m_id=c.at("id"),.m_content_id=c.at("content_id"),
                .m_size=c.at("size"),.m_transfer_end=c.at("transfer_end"),.m_load_end=c.at("load_end"),.m_link_end=c.at("link_end")});
        file->m_file_load_end_progress=prefix(file->m_chunk_arr,[](const auto& c) { return c.m_link_end; });
        file->m_chunk_load_end_progress=prefix(file->m_chunk_arr,[](const auto& c) { return c.m_load_end; });
        file->m_chunk_transfer_end_progress=prefix(file->m_chunk_arr,[](const auto& c) { return c.m_transfer_end; });
        // Begin counters describe dispatched work, not confirmed work. Rebuild
        // them from durable flags so all interrupted requests become runnable.
        file->m_chunk_load_begin_progress=file->m_chunk_load_end_progress;
    }
    else
    {
        auto& folder=static_cast<upload_folder_t&>(entity);
        if (s.at("main_id")!=folder.m_main_id) throw std::runtime_error{"Saved upload identity changed."};
        folder.m_next_id=s.at("next_id");
        folder.m_folder_load_begin_progress=s.at("folder_load_begin_progress");
        folder.m_entity_load_end=s.at("entity_load_end").get<std::vector<bool>>();
        if (folder.m_entity_load_end.size()!=folder.m_entity_arr.size())
            throw std::runtime_error{"Saved upload directory changed."};
        folder.m_folder_load_end_progress=prefix(folder.m_entity_load_end,[](bool done) { return done; });
        folder.m_load_end=s.at("load_end");
        folder.m_pages=pages_restore(s.at("pages"));
    }
}
auto upload_progress_ready(upload_task_t& task) -> void
{
    task.m_progress=0;
    for (const auto& entity:task.m_entity_arr)
        if (const auto* file=dynamic_cast<const upload_file_t*>(entity.get()))
            for (const auto& c:file->m_chunk_arr) if (c.m_transfer_end) task.m_progress+=c.m_size;
    task.m_begin_progress=std::ranges::find_if(task.m_entity_arr,[](const auto& e) { return !upload_finished(*e); });
    task.m_end_progress=task.m_begin_progress;
}

auto download_progress_save(download_file_t& file) -> void
{
    if (!file.m_task->m_journal) return;
    auto s=json{{"size",file.m_size},{"source_id",file.m_source_id},{"next_id",file.m_next_id},
        {"written",file.m_written},{"filesystem_progress",file.m_filesystem_progress},{"complete",file.m_complete},
        {"chunk_load_begin_progress",file.m_chunk_load_begin_progress},
        {"chunk_load_end_progress",file.m_chunk_load_end_progress},
        {"chunk_transfer_begin_progress",file.m_chunk_transfer_begin_progress},{"chunks",json::array()}};
    for (const auto& c:file.m_chunk_arr)
    {
        auto entry=json{{"id",c.m_id},{"content_id",c.m_content_id},{"loaded",c.m_loaded},
                        {"received",c.m_received},{"response_key",c.m_response_key}};
        if (file.m_task->m_session)
        {
            entry["account"]=file.m_task->m_session->owner(c.m_id);
            if (!c.m_content_id.empty()) entry["content_account"]=file.m_task->m_session->owner(c.m_content_id.front());
        }
        s["chunks"].push_back(std::move(entry));
    }
    if (file.m_task->m_session) s["next_account"]=file.m_task->m_session->owner(file.m_next_id);
    file.m_task->m_journal->progress("download:"+file.m_path_local,s);
}
auto download_progress_restore(download_file_t& file) -> bool
{
    if (!file.m_task->m_journal) return false;
    const auto stored=file.m_task->m_journal->progress("download:"+file.m_path_local);
    if (!stored) return false;
    const auto& s=*stored;
    if (s.at("source_id")!=file.m_source_id) throw std::runtime_error{"Saved download identity changed."};
    file.m_size=s.at("size"); file.m_next_id=s.at("next_id");
    file.m_written=s.at("written");
    file.m_filesystem_progress=s.at("filesystem_progress"); file.m_complete=s.at("complete");
    for (const auto& c:s.at("chunks"))
    {
        file.m_chunk_arr.push_back({.m_id=c.at("id"),.m_content_id=c.at("content_id"),
            .m_loaded=c.at("loaded"),.m_received=c.at("received"),.m_response_key=c.at("response_key")});
        if (file.m_task->m_session)
        {
            file.m_task->m_session->remember(c.at("id"),c.at("account"),"chunk");
            for (const auto& id:c.at("content_id")) file.m_task->m_session->remember(id,c.at("content_account"),"data");
        }
    }
    if (file.m_task->m_session && file.m_next_id!="null")
        file.m_task->m_session->remember(file.m_next_id,s.at("next_account"),"file");
    if (file.m_filesystem_progress>file.m_chunk_arr.size() || file.m_written>file.m_size)
        throw std::runtime_error{"Invalid saved download progress."};
    for (std::size_t i=file.m_filesystem_progress;i<file.m_chunk_arr.size();++i)
    {
        auto& c=file.m_chunk_arr[i];
        if (!c.m_received) continue;
        const auto response=file.m_task->m_journal->response(c.m_response_key);
        if (!response) throw std::runtime_error{"Missing saved out-of-order download response."};
        const auto parts=json::parse(*response);
        for (std::size_t part{};part<c.m_content_id.size();++part)
            c.m_content.push_back(json::parse(parts.at(std::to_string(part)).get<std::string>()).at("description"));
    }
    file.m_chunk_load_end_progress=prefix(file.m_chunk_arr,[](const auto& c) { return c.m_loaded; });
    file.m_chunk_load_begin_progress=file.m_chunk_load_end_progress;
    file.m_chunk_transfer_begin_progress=file.m_filesystem_progress;
    while (file.m_chunk_transfer_begin_progress<file.m_chunk_arr.size() &&
           file.m_chunk_arr[file.m_chunk_transfer_begin_progress].m_received) ++file.m_chunk_transfer_begin_progress;
    file.m_idle=!file.m_complete;
    return true;
}
auto download_manifest_save(download_task_t& task) -> void
{
    if (!task.m_journal) return;
    auto s=json{{"directories",task.m_directories},{"files",json::array()}};
    for (const auto& f:task.m_file_arr) s["files"].push_back({{"path",f.m_path_local},{"id",f.m_source_id}});
    if (task.m_session) s["routing"]=task.m_session->routing();
    task.m_journal->progress("download-manifest",s);
}
auto download_manifest_restore(google_easy_t& easy, download_task_t& task) -> bool
{
    if (!task.m_journal) return false;
    const auto s=task.m_journal->progress("download-manifest");
    if (!s) return false;
    if (task.m_session && s->contains("routing")) task.m_session->restore_routing(s->at("routing"));
    task.m_directories=s->at("directories").get<std::vector<std::string>>();
    for (const auto& path:task.m_directories) std::filesystem::create_directories(path);
    for (const auto& f:s->at("files")) file_download(&easy,&task,f.at("id"),f.at("path").get<std::string>());
    return true;
}
auto task_progress_save(load_task_t& task) -> void
{
    if (!task.m_journal) return;
    if (const auto* upload=dynamic_cast<const upload_task_t*>(&task))
        task.m_journal->progress("task",{{"progress",upload->m_progress},{"size",upload->m_size},
            {"begin_progress",upload->m_begin_progress-upload->m_entity_arr.begin()},
            {"end_progress",upload->m_end_progress-upload->m_entity_arr.begin()}});
    else if (const auto* download=dynamic_cast<const download_task_t*>(&task))
        task.m_journal->progress("task",{{"progress",download->m_progress},{"size",download->m_size},
            {"idle_first",download->m_idle_first-download->m_file_arr.begin()},
            {"files_remaining",download->m_files_remaining}});
}
