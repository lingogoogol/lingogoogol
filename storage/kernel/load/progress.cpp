module;
#include <nlohmann/json.hpp>
module storage.kernel.load;
import std;
import storage.local;
import storage.encode;

namespace
{
    using nlohmann::json;
    auto pages_restore(const json& saved) -> std::vector<upload_page_t>
    {
        std::vector<upload_page_t> result;
        for (const auto& p:saved)
            result.push_back({p.at("id"),p.at("next"),p.at("begin"),p.at("count"),p.at("complete"),false});
        return result;
    }
    auto page_record(const upload_page_t& p) -> json
    {return {{"id",p.id},{"next",p.next},{"begin",p.begin},{"count",p.count},{"complete",p.complete}};}
    auto restore_pages(const json& saved,upload_entity_t& entity) -> std::vector<upload_page_t>
    {
        if(saved.contains("pages"))return pages_restore(saved.at("pages"));
        auto pages=json::array();
        for(std::size_t i=0;i<saved.value("page_count",std::size_t{});++i)
        {
            const auto page=entity.m_task->m_journal->progress("upload-page:"+entity.m_path_local+":"+std::to_string(i));
            if(!page)throw std::runtime_error{"Missing saved upload page."};
            pages.push_back(*page);
        }
        return pages_restore(pages);
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
auto upload_progress_save(upload_entity_t& entity,std::size_t changed_begin,std::size_t changed_count) -> void
{
    if (!entity.m_task->m_journal) return;
    auto saved=json{{"main_id",dynamic_cast<upload_file_t*>(&entity) ?
        static_cast<upload_file_t&>(entity).m_main_id : static_cast<upload_folder_t&>(entity).m_main_id}};
    if (auto* file=dynamic_cast<upload_file_t*>(&entity))
    {
        saved["next_id"]=file->m_next_id;
        saved["chunk_cover_progress"]=file->m_chunk_cover_progress;
        saved["file_load_begin_progress"]=file->m_file_load_begin_progress;
        saved["file_load_end_progress"]=file->m_file_load_end_progress;
        saved["chunk_load_begin_progress"]=file->m_chunk_load_begin_progress;
        saved["chunk_load_end_progress"]=file->m_chunk_load_end_progress;
        saved["chunk_transfer_end_progress"]=file->m_chunk_transfer_end_progress;
        saved["load_end"]=file->m_load_end;
        for(auto i=file->m_saved_page_count;i<file->m_pages.size();++i)
            entity.m_task->m_journal->progress("upload-page:"+entity.m_path_local+":"+std::to_string(i),page_record(file->m_pages[i]));
        file->m_saved_page_count=file->m_pages.size();saved["page_count"]=file->m_pages.size();
        saved["page_key"]="upload-page:"+entity.m_path_local+":";
        // Chunk records change independently. Do not serialize every prior
        // chunk again for each response (quadratic work on long transfers).
        const auto save_chunk=[&](std::size_t index)
        {
            const auto& c=file->m_chunk_arr.at(index);
            entity.m_task->m_journal->progress("upload-chunk:"+entity.m_path_local+":"+std::to_string(index),
                {{"id",c.m_id},{"content_id",c.m_content_id},{"size",c.m_size},
                 {"transfer_end",c.m_transfer_end},{"load_end",c.m_load_end},{"link_end",c.m_link_end}});
        };
        for(auto i=file->m_saved_chunk_count;i<file->m_chunk_arr.size();++i)save_chunk(i);
        if(changed_begin!=SIZE_MAX)
            for(auto i=changed_begin;i<std::min(file->m_chunk_arr.size(),changed_begin+changed_count);++i)
                if(i<file->m_saved_chunk_count)save_chunk(i);
        file->m_saved_chunk_count=file->m_chunk_arr.size();
        saved["chunk_count"]=file->m_chunk_arr.size();
        saved["chunk_key"]="upload-chunk:"+entity.m_path_local+":";
    }
    else
    {
        auto& folder=static_cast<upload_folder_t&>(entity);
        saved["next_id"]=folder.m_next_id;
        saved["folder_load_begin_progress"]=folder.m_folder_load_begin_progress;
        saved["folder_load_end_progress"]=folder.m_folder_load_end_progress;
        saved["entity_count"]=folder.m_entity_load_end.size();
        saved["load_end"]=folder.m_load_end;
        for(auto i=folder.m_saved_page_count;i<folder.m_pages.size();++i)
            entity.m_task->m_journal->progress("upload-page:"+entity.m_path_local+":"+std::to_string(i),page_record(folder.m_pages[i]));
        folder.m_saved_page_count=folder.m_pages.size();saved["page_count"]=folder.m_pages.size();
        saved["page_key"]="upload-page:"+entity.m_path_local+":";
    }
    entity.m_task->m_journal->progress("upload:"+entity.m_path_local,saved);
}
auto upload_page_completed(upload_entity_t& entity,std::size_t begin) -> void
{
    auto* file=dynamic_cast<upload_file_t*>(&entity);
    auto& pages=file ? file->m_pages : static_cast<upload_folder_t&>(entity).m_pages;
    const auto page=std::ranges::lower_bound(pages,begin,{},&upload_page_t::begin);
    if(page==pages.end() || page->begin!=begin)throw std::runtime_error{"Missing upload page."};
    page->complete=true;
    if(entity.m_task->m_journal)
        entity.m_task->m_journal->progress("upload-page:"+entity.m_path_local+":"+std::to_string(page-pages.begin()),page_record(*page));
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
        file->m_pages=restore_pages(s,entity);
        file->m_saved_page_count=file->m_pages.size();
        auto chunks=s.value("chunks",json::array());
        if(s.contains("chunk_count"))
            for(std::size_t i=0;i<s.at("chunk_count").get<std::size_t>();++i)
            {
                const auto chunk=entity.m_task->m_journal->progress("upload-chunk:"+entity.m_path_local+":"+std::to_string(i));
                if(!chunk)throw std::runtime_error{"Missing saved upload chunk."};
                chunks.push_back(*chunk);
            }
        for (const auto& c:chunks)
            file->m_chunk_arr.push_back({.m_file=file,.m_id=c.at("id"),.m_content_id=c.at("content_id"),
                .m_size=c.at("size"),.m_transfer_end=c.at("transfer_end"),.m_load_end=c.at("load_end"),.m_link_end=c.at("link_end")});
        file->m_saved_chunk_count=file->m_chunk_arr.size();
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
        folder.m_pages=restore_pages(s,entity);folder.m_saved_page_count=folder.m_pages.size();
        if(s.contains("entity_load_end"))folder.m_entity_load_end=s.at("entity_load_end").get<std::vector<bool>>();
        else
        {
            folder.m_entity_load_end.assign(s.at("entity_count").get<std::size_t>(),false);
            for(const auto& page:folder.m_pages)if(page.complete)
                for(auto i=page.begin;i<page.begin+page.count;++i)folder.m_entity_load_end.at(i)=true;
        }
        if (folder.m_entity_load_end.size()!=folder.m_entity_arr.size())
            throw std::runtime_error{"Saved upload directory changed."};
        folder.m_folder_load_end_progress=prefix(folder.m_entity_load_end,[](bool done) { return done; });
        folder.m_load_end=s.at("load_end");
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

auto download_progress_save(download_file_t& file,std::size_t changed_begin,std::size_t changed_count) -> void
{
    if (!file.m_task->m_journal) return;
    auto s=json{{"size",file.m_size},{"source_id",file.m_source_id},{"next_id",file.m_next_id},
        {"written",file.m_written},{"filesystem_progress",file.m_filesystem_progress},{"complete",file.m_complete},
        {"chunk_load_begin_progress",file.m_chunk_load_begin_progress},
        {"chunk_load_end_progress",file.m_chunk_load_end_progress},
        {"chunk_transfer_begin_progress",file.m_chunk_transfer_begin_progress},{"chunk_count",file.m_chunk_arr.size()}};
    const auto save_chunk=[&](std::size_t index)
    {
        const auto& c=file.m_chunk_arr.at(index);
        auto entry=json{{"id",c.m_id},{"content_id",c.m_content_id},{"loaded",c.m_loaded},
                        {"received",c.m_received},{"response_key",c.m_response_key}};
        if (file.m_task->m_session)
        {
            entry["account"]=file.m_task->m_session->owner(c.m_id);
            if (!c.m_content_id.empty()) entry["content_account"]=file.m_task->m_session->owner(c.m_content_id.front());
        }
        file.m_task->m_journal->progress("download-chunk:"+file.m_path_local+":"+std::to_string(index),entry);
    };
    for(auto i=file.m_saved_chunk_count;i<file.m_chunk_arr.size();++i)save_chunk(i);
    if(changed_begin!=SIZE_MAX)
        for(auto i=changed_begin;i<std::min(file.m_chunk_arr.size(),changed_begin+changed_count);++i)
            if(i<file.m_saved_chunk_count)save_chunk(i);
    file.m_saved_chunk_count=file.m_chunk_arr.size();
    s["chunk_key"]="download-chunk:"+file.m_path_local+":";
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
    auto chunks=s.value("chunks",json::array());
    if(s.contains("chunk_count"))
        for(std::size_t i=0;i<s.at("chunk_count").get<std::size_t>();++i)
        {
            const auto chunk=file.m_task->m_journal->progress("download-chunk:"+file.m_path_local+":"+std::to_string(i));
            if(!chunk)throw std::runtime_error{"Missing saved download chunk."};
            chunks.push_back(*chunk);
        }
    for (const auto& c:chunks)
    {
        file.m_chunk_arr.push_back({.m_id=c.at("id"),.m_content_id=c.at("content_id"),
            .m_loaded=c.at("loaded"),.m_received=c.at("received"),.m_response_key=c.at("response_key")});
        if (file.m_task->m_session)
        {
            file.m_task->m_session->remember(c.at("id"),c.at("account"),"chunk");
            for (const auto& id:c.at("content_id")) file.m_task->m_session->remember(id,c.at("content_account"),"data");
        }
    }
    file.m_saved_chunk_count=file.m_chunk_arr.size();
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
            c.m_content.push_back(content_blob_decode(json::parse(parts.at(std::to_string(part)).get<std::string>())));
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
    for (const auto& path:task.m_directories) storage_local::create_directories(path);
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
