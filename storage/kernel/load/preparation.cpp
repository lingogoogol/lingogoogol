module;
#include <nlohmann/json.hpp>
module storage.kernel.load;
import std;

namespace
{
    auto prepare_id(upload_task_t& task, blob_queue_t* fallback, const std::string& path,
                    const std::string& type, const std::string& source_key,
                    const nlohmann::json& source) -> std::string
    {
        if (task.m_control->cancelled) throw std::runtime_error{"Task cancelled."};
        if (task.m_journal)
        {
            const auto saved=task.m_journal->field(source_key);
            if (saved && *saved!=source)
                throw std::runtime_error{"Upload source changed since the saved task: "+path};
        }
        const auto key=type+":"+path;
        std::string address;
        auto fields=nlohmann::json{{source_key,source}};
        if (task.m_session)
        {
            const auto field="owner:"+key;
            const auto saved=task.m_journal ? task.m_journal->field(field) : std::nullopt;
            address=saved ? saved->get<std::string>() : task.m_session->choose(1);
            fields[field]=address;
        }
        auto reserve=[&](std::size_t count)
        {
            auto* queue=task.m_session ? task.m_session->queues.at(address).get() : fallback;
            if (!queue) throw std::runtime_error{"No account reserve is available."};
            return queue->get(count,&task.m_control->cancelled);
        };
        // One entity per durable record: source, owner, ID and routing commit
        // together. Until it succeeds, this entity cannot reach the scheduler.
        // The queue commits first; an interruption between the two commits can
        // orphan an unused reserve, but cannot reuse an uploaded ID.
        const auto id=(task.m_journal ? task.m_journal->allocate(key,1,reserve,fields,address,type) : reserve(1)).front();
        if (task.m_session) task.m_session->remember(id,address,type);
        return id;
    }
}

auto file_upload
(
	blob_queue_t* blob_queue,
	upload_task_t* task,
	std::filesystem::path path_local,
	upload_folder_t* parent
)
-> std::size_t
{
	auto file{ std::make_shared<upload_file_t>() };
	file->m_task = task;
	file->m_parent = parent;
	file->m_type = "file";
	file->m_name = path_local.filename().string();
    file->m_path_local = path_local.string();
    file->m_size = std::filesystem::file_size(path_local);
    file->m_main_id=prepare_id(*task,blob_queue,path_local.string(),"file","source:"+path_local.string(),
        {{"size",file->m_size},{"mtime",std::filesystem::last_write_time(path_local).time_since_epoch().count()}});
    file->m_next_id=file->m_main_id;
    task->m_size += file->m_size;
    file->m_self_i = task->m_entity_arr.size();
    upload_progress_restore(*file);
    task->m_entity_arr.emplace_back(file);
    return file->m_self_i;
}

auto folder_upload
(
	blob_queue_t* blob_queue,
	upload_task_t* task,
	std::filesystem::path path_local,
	upload_folder_t* parent
)
-> std::size_t
{
	auto folder{ std::make_shared<upload_folder_t>() };
	folder->m_task = task;
	folder->m_parent = parent;
	folder->m_type = "folder";
	folder->m_name = path_local.filename().string();
    folder->m_path_local=path_local.string();
	folder->m_self_i = task->m_entity_arr.size();
    task->m_entity_arr.emplace_back(folder);
    std::vector<std::filesystem::path> children;
    for (const auto& child : std::filesystem::directory_iterator{path_local})
    {
        if (task->m_control->cancelled) throw std::runtime_error{"Task cancelled."};
        children.push_back(child.path());
    }
    std::ranges::sort(children);
    std::vector<std::string> names;
    for (const auto& child : children) names.push_back(child.filename().string());
    folder->m_main_id=prepare_id(*task,blob_queue,path_local.string(),"folder","children:"+path_local.string(),names);
    folder->m_next_id=folder->m_main_id;
    for (const auto& child : children)
    {
        if (std::filesystem::is_regular_file(child))
            folder->m_entity_arr.push_back(file_upload(blob_queue, task, child, folder.get()));
        else if (std::filesystem::is_directory(child))
            folder->m_entity_arr.push_back(folder_upload(blob_queue, task, child, folder.get()));
    }
    folder->m_entity_load_end.resize(folder->m_entity_arr.size());
    upload_progress_restore(*folder);
    return folder->m_self_i;
}
