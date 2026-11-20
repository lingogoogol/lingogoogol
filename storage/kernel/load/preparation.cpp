module;
#include <nlohmann/json.hpp>
module storage.kernel.load;
import std;
import storage.local;

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
	file->m_name = storage_local::name(path_local.string());
    file->m_path_local = path_local.string();
    file->m_size = storage_local::size(path_local.string());
    file->m_main_id=prepare_id(*task,blob_queue,path_local.string(),"file","source:"+path_local.string(),
        {{"size",file->m_size},{"mtime",storage_local::modified(path_local.string())}});
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
	folder->m_name = storage_local::name(path_local.string());
    folder->m_path_local=path_local.string();
	folder->m_self_i = task->m_entity_arr.size();
    task->m_entity_arr.emplace_back(folder);
    const auto children{storage_local::children(path_local.string())};
    if (task->m_control->cancelled) throw std::runtime_error{"Task cancelled."};
    std::vector<std::string> names;
    for (const auto& child : children) names.push_back(storage_local::name(child.path));
    folder->m_main_id=prepare_id(*task,blob_queue,path_local.string(),"folder","children:"+path_local.string(),names);
    folder->m_next_id=folder->m_main_id;
    for (const auto& child : children)
    {
        if (child.kind == storage_local::kind_t::file)
            folder->m_entity_arr.push_back(file_upload(blob_queue, task, child.path, folder.get()));
        else if (child.kind == storage_local::kind_t::directory)
            folder->m_entity_arr.push_back(folder_upload(blob_queue, task, child.path, folder.get()));
    }
    for (const auto index : folder->m_entity_arr)
    {
        const auto& child = task->m_entity_arr[index];
        const auto size = child->m_type == "file" ? static_cast<upload_file_t&>(*child).m_size :
                                                   static_cast<upload_folder_t&>(*child).m_size;
        if (size > std::numeric_limits<std::uint64_t>::max()-folder->m_size)
            throw std::runtime_error{"Folder size overflow."};
        folder->m_size += size;
    }
    folder->m_entity_load_end.resize(folder->m_entity_arr.size());
    upload_progress_restore(*folder);
    return folder->m_self_i;
}
