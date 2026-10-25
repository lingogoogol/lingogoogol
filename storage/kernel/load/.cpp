module;

#include <nlohmann/json.hpp>

module storage.kernel.load;

import std;

import external.curl;

import lgo.dev;
import lgo.io;
import lgo.GUI;

import storage.encode;
import storage.http;
import storage.google;

auto list_folder_content(google_easy_t& easy, std::string* pos) -> nlohmann::json
{
    std::string next{ *pos };
	//Use `=` instead of `{}` when initializing `nlohmann::json` from a single `nlohmann::json` expression:
	//brace-initialization `nlohmann::json{ expr }` selects the `std::initializer_list` constructor and
	//wraps `expr` into a 1-element JSON array `[expr]`, causing runtime `type_error.305` on key lookup.
	nlohmann::json entity_arr = nlohmann::json::array();
	while (next != "null")
	{
		nlohmann::json response_data = blob_get_run(easy, next, "name,description");
		nlohmann::json response_content = nlohmann::json::parse(response_data["description"].get<std::string>());
		entity_arr.insert(entity_arr.end(), response_content.begin(), response_content.end());
		next = nlohmann::json::parse(response_data["name"].get<std::string>())["next"];
	}
	return entity_arr;
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
	file->m_main_id = blob_queue->get();
	file->m_next_id = file->m_main_id;
	file->m_path_local = path_local.string();
	file->m_size = std::filesystem::file_size(path_local);
	task->m_size += file->m_size;
	file->m_self_i = task->m_entity_arr.size();
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
	folder->m_main_id = blob_queue->get();
	folder->m_next_id = folder->m_main_id;
	folder->m_self_i = task->m_entity_arr.size();
	task->m_entity_arr.emplace_back(folder);
	for (std::filesystem::directory_iterator i{ path_local }; i != std::filesystem::directory_iterator{}; ++i)
	{
		if (i->is_regular_file())
		{
			folder->m_entity_arr.emplace_back(file_upload(blob_queue, task, i->path(), folder.get()));
		}
		else if (i->is_directory())
		{
			folder->m_entity_arr.emplace_back(folder_upload(blob_queue, task, i->path(), folder.get()));
		}
	}
	folder->m_entity_load_end.resize(folder->m_entity_arr.size());
	return folder->m_self_i;
}

auto file_download
(
	google_easy_t* easy,
	download_task_t* task,
	std::string pos,
	std::filesystem::path path_local
)
-> void
{
	download_file_t file{};
	file.m_task = task;
	file.m_name = path_local.string();
    file.m_path_local = path_local.string();
	file.m_size = nlohmann::json::parse(blob_get_name_run(*easy, pos))["size"];
	file.m_next_id = pos;
	task->m_size += file.m_size;
	task->m_file_arr.emplace_back(file);
	return;
}

auto folder_download
(
	google_easy_t* easy,
	download_task_t* task,
	std::string pos,
	std::filesystem::path path_local
)
-> void
{
	auto obj_list{ list_folder_content(*easy, &pos) };
	std::filesystem::create_directories(path_local);
	for (auto i : obj_list)
	{
		if (i["type"] == "folder")
		{
			folder_download
			(
				easy,
				task,
				i["id"],
				path_local / i["name"]
			);
		}
		else if (i["type"] == "file")
		{
			file_download
			(
				easy,
				task,
				i["id"],
				path_local / i["name"]
			);
		}
		else
		{
			throw lgo::internal_error_t{ "obj type" };
		}
	}
	return;
}

auto folder_data_add
(
    google_easy_t& easy,
    std::string last_id,
    std::string name,
    std::string parent,
    nlohmann::json content
)
-> std::string
{
	std::string id
	{
		blob_upload_run
		(
			easy,
			nlohmann::json
			{
				{ "name", name },
				{ "parent", parent },
				{ "next", "null" }
			}
			.dump(),
			content.dump()
		)
	};
	if (last_id != "null")
	{
		nlohmann::json last_json = nlohmann::json::parse(blob_get_name_run(easy, last_id));
		last_json["next"] = id;
		blob_update_name_run(easy, last_id, last_json.dump());
	}
	return id;
}

auto folder_add
(
    google_easy_t& easy,
    std::string pos,
    nlohmann::json entity
)
-> void
{
	nlohmann::json folder{};
	nlohmann::json folder_name{};
	while (true)
	{
        folder = blob_get_run(easy, pos, "name,description");
        folder_name = nlohmann::json::parse(folder["name"].get<std::string>());
        if (folder_name["next"] == "null")
        {
            break;
        }
        pos = folder_name["next"];
	}
	nlohmann::json folder_content = nlohmann::json::parse(folder["description"].get<std::string>());

	folder_content.emplace_back(entity);
	std::string content{ folder_content.dump() };
    //Create a new folder blob if its entity count will exceed 100 or the content is too long.
    //It can occur when the name of its entities are too long for example.
	if (folder_content.size() > 100 || content.size() > content_size_max)
	{
		nlohmann::json array = nlohmann::json::array();
		array.push_back(entity);
		std::string data_id
        {
            folder_data_add
            (
                easy,
                pos,
                folder_name["name"],
                folder_name["parent"],
                array
            )
        };
		folder_name["next"] = data_id;
		blob_update_name_run(easy, pos, folder_name.dump());
	}
	else
	{
		blob_update_content_run(easy, pos, content);
	}
	return;
}

auto folder_root_create(google_easy_t& easy) -> std::string
{
	return blob_upload_run
	(
		easy,
		nlohmann::json
		{
			{ "name", folder_root_name },
			{ "parent", "null" },
			{ "next", "null" }
		}
		.dump(),
		nlohmann::json::array().dump()
	);
}

//This function looks for a task that has idle file and attempts to start a transfer
// and returns a boolean indicating whether it successfully started a transfer.
//If it returns false, it means that there are no transfers to start for now
// either because we must wait for other transfers to end to start a transfer
// or there is simply no tasks to run.
//This function does not modify the easy object if there are no transfers to run.
auto transfer_begin(load_state_t* state, curl_multi_t& multi, curl_easy_t&& easy, blob_queue_t* blob_queue) -> bool
{
    using namespace std::chrono_literals;

    for (auto& task_ptr : state->m_task_list)
    {
        if (task_ptr->m_type == "upload")
        {
            auto upload_task_ptr{ std::dynamic_pointer_cast<upload_task_t>(task_ptr) };
            if (upload_task_ptr->m_prepared.valid())
            {
                if (upload_task_ptr->m_prepared.wait_for(0s) != std::future_status::ready)
                {
                    continue;
                }
                //Consume the future so preparation failures cannot be mistaken for success.
                upload_task_ptr->m_prepared.get();
            }
            auto entity_i{ upload_task_ptr->m_begin_progress };
            if (entity_i == upload_task_ptr->m_entity_arr.end())
            {
                continue;
            }
            if ((*entity_i)->m_type == "file")
            {
                auto file_ptr{ dynamic_cast<upload_file_t*>(entity_i->get()) };
                if (file_ptr->m_chunk_cover_progress < file_ptr->m_size)
                {
                    upload_chunk_transfer_begin(multi, std::move(easy), blob_queue, file_ptr);
                }
                else if (file_ptr->m_chunk_arr.size() > file_ptr->m_chunk_load_begin_progress)
                {
                    upload_chunk_load_begin(multi, std::move(easy), file_ptr);
                }
                else if (file_ptr->m_next_id != "null")
                {
                    upload_file_load_begin(multi, std::move(easy), blob_queue, file_ptr);
                }
                else
                {
                    throw;
                }
            }
            else if ((*entity_i)->m_type == "folder")
            {
                auto folder_ptr{ dynamic_cast<upload_folder_t*>(entity_i->get()) };
                upload_folder_load_begin(multi, std::move(easy), blob_queue, folder_ptr);
            }
            else
            {
                throw;
            }
            return true;
        }
        else if (task_ptr->m_type == "download")
        {
            auto download_task_ptr{ std::dynamic_pointer_cast<download_task_t>(task_ptr) };
            if (download_task_ptr->m_prepared.wait_for(0s) != std::future_status::ready)
            {
                continue;
            }
            auto file_i{ download_task_ptr->m_idle_first };
            if (file_i == download_task_ptr->m_file_arr.end())
            {
                continue;
            }
            if (file_i->m_next_id != "null" && !file_i->m_file_loading)
            {
                download_file_load_begin(multi, std::move(easy), file_i);
            }
            else if (file_i->m_chunk_load_begin_progress < file_i->m_chunk_arr.size())
            {
                download_chunk_load_begin(multi, std::move(easy), file_i);
            }
            else if (file_i->m_chunk_transfer_begin_progress < file_i->m_chunk_load_end_progress)
            {
                download_chunk_transfer_begin
                (
                    multi,
                    std::move(easy),
                    file_i->m_chunk_arr[file_i->m_chunk_transfer_begin_progress]
                );
            }
            else
            {
                throw;
            }
            return true;
        }
        else
        {
            throw;
        }
    }
	return false;
}

//Completes other tasks that depends on the network transfer
// and updates the loading state.
auto transfer_end(load_state_t* state, std::unique_ptr<curl_easy_t>& easy) -> void
{
    state->m_end[easy->interface_get()](easy);
    state->m_end.erase(easy->interface_get());
	return;
}

//Check if the upload task is complete and remove the task from load state when appropriate.
auto upload_complete(upload_entity_t* entity) -> void
{
    upload_task_t* task{ entity->m_task };

    //Update task progress.
    if (entity != task->m_end_progress->get())
    {
        return;
    }
    while (task->m_end_progress != task->m_entity_arr.end())
    {
        entity = task->m_end_progress->get();
        if (entity->m_type == "file")
        {
            auto file{ dynamic_cast<upload_file_t*>(entity) };
            if
            (
                file->m_load_end &&
                file->m_chunk_cover_progress == file->m_size &&
                file->m_file_load_end_progress == file->m_chunk_arr.size() &&
                file->m_chunk_load_end_progress == file->m_chunk_arr.size() &&
                file->m_chunk_transfer_end_progress == file->m_chunk_arr.size()
            )
            {
                ++task->m_end_progress;
            }
            else
            {
                break;
            }
        }
        else if (entity->m_type == "folder")
        {
            auto folder{ dynamic_cast<upload_folder_t*>(entity) };
            if (folder->m_load_end && folder->m_folder_load_end_progress == folder->m_entity_arr.size())
            {
                ++task->m_end_progress;
            }
            else
            {
                break;
            }
        }
        else
        {
            throw;
        }
    }

    //Remove the task from load state if it is finished.
    if (task->m_end_progress == task->m_entity_arr.end())
    {
        task->m_state->m_progress_column->remove_object(task->m_progress_div);
        task->m_state->m_task_list.erase(task->m_self_i);
    }
    return;
}

auto upload_file_load_begin
(
    curl_multi_t& multi,
    curl_easy_t&& easy,
    blob_queue_t* blob_queue,
    upload_file_t* file
)
-> void
{
	blob_opt_t blob
    {
        .m_id{ file->m_next_id }
    };
    std::size_t chunk_count{};
    if (file->m_chunk_arr.size() - file->m_file_load_begin_progress > 100)
    {
        file->m_next_id = blob_queue->get();
        chunk_count = 100;
    }
    else
    {
        file->m_next_id = "null";
        chunk_count = file->m_chunk_arr.size() - file->m_file_load_begin_progress;
    }
	blob.m_name =
		nlohmann::json
		{
			{ "name", std::filesystem::path{ file->m_path_local }.filename().string() },
			{ "size", file->m_size },
			{ "parent", file->m_parent->m_main_id },
			{ "next", file->m_next_id }
		};
	blob.m_content = nlohmann::json::array();
    for (std::size_t i{ 0 }; i < chunk_count; ++i)
    {
        blob.m_content->push_back(file->m_chunk_arr[file->m_file_load_begin_progress + i].m_id);
    }
    google_easy_t easy_google{ std::move(easy) };
    easy_google.path_set("/drive/v3");
	blob_update(easy_google, blob);
    //Get the key to `m_state->m_end` first before `easy_google` is moved.
    CURL* key{ easy_google.interface_get() };
    multi.add(std::move(easy_google));
    
    file->m_file_load_begin_progress += chunk_count;
    if (file->m_file_load_begin_progress == file->m_chunk_arr.size())
    {
        ++file->m_task->m_begin_progress;
    }
    file->m_task->m_state->m_end.emplace
    (
        key,
        std::bind
        (
            upload_file_load_end,
            std::placeholders::_1,
            file,
            file->m_file_load_begin_progress - chunk_count,
            chunk_count
        )
    );
    return;
}

auto upload_file_load_end
(
    std::unique_ptr<curl_easy_t>&,
    upload_file_t* file,
    std::uint64_t begin_i,
    std::uint64_t chunk_count
)
-> void
{
    for (std::uint64_t i{ 0 }; i < chunk_count; ++i)
    {
        file->m_chunk_arr[begin_i + i].m_link_end = true;
    }
    while
    (
        file->m_file_load_end_progress < file->m_chunk_arr.size() &&
        file->m_chunk_arr[file->m_file_load_end_progress].m_link_end
    )
    {
        ++file->m_file_load_end_progress;
    }
    if (begin_i + chunk_count == file->m_chunk_arr.size())
    {
        file->m_load_end = true;
    }
    upload_complete(file);
    return;
}

auto upload_chunk_load_begin(curl_multi_t& multi, curl_easy_t&& easy, upload_file_t* file) -> void
{
    google_batch_t batch{ std::move(easy) };
    batch.path_set("/drive/v3");
    std::uint64_t chunk_count{ std::min(file->m_chunk_arr.size() - file->m_chunk_load_begin_progress, 100ull) };
    for (std::uint64_t i{ 0 }; i < chunk_count; ++i)
    {
        upload_chunk_t& chunk{ file->m_chunk_arr[file->m_chunk_load_begin_progress + i] };
        nlohmann::json id_array = nlohmann::json::array();
        for (std::uint64_t j{ 0 }; j < chunk.m_content_id.size(); ++j)
        {
            id_array.push_back(chunk.m_content_id[j]);
        }
        blob_opt_t blob
        {
            .m_id{ chunk.m_id },
            .m_name
            {
                nlohmann::json
                {
                    { "parent", file->m_main_id }
                }
            },
            .m_content{ id_array }
        };
        blob_update(batch, blob);
    }
    batch.method_post();
    //Get the key to `m_state->m_end` first before `batch` is moved.
    CURL* key{ batch.interface_get() };
    multi.add(std::move(batch));

    file->m_task->m_state->m_end.emplace
    (
        key,
        std::bind
        (
            upload_chunk_load_end,
            std::placeholders::_1,
            file,
            file->m_chunk_load_begin_progress,
            chunk_count
        )
    );
    file->m_chunk_load_begin_progress += chunk_count;
    return;
}

auto upload_chunk_load_end
(
    std::unique_ptr<curl_easy_t>&,
    upload_file_t* file,
    std::uint64_t begin_i,
    std::uint64_t chunk_count
)
-> void
{
    for (std::uint64_t i{ 0 }; i < chunk_count; ++i)
    {
        file->m_chunk_arr[begin_i + i].m_load_end = true;
    }
    while
    (
        file->m_chunk_load_end_progress < file->m_chunk_arr.size() &&
        file->m_chunk_arr[file->m_chunk_load_end_progress].m_load_end
    )
    {
        ++file->m_chunk_load_end_progress;
    }
    upload_complete(file);
    return;
}

auto upload_chunk_transfer_begin
(
    curl_multi_t& multi,
    curl_easy_t&& easy,
    blob_queue_t* blob_queue,
    upload_file_t* file
)
-> void
{
    auto& chunk
    {
        file->m_chunk_arr.emplace_back
        (
            upload_chunk_t
            {
                .m_file{ file },
                .m_id{ blob_queue->get() }
            }
        )
    };
    std::uint64_t batch_size{ std::min(file->m_size - file->m_chunk_cover_progress, content_size_max_base64 * 100) };
    //Divide and round up.
    std::uint64_t content_count{ (batch_size + content_size_max_base64 - 1) / content_size_max_base64 };
    chunk.m_content_id = blob_queue->get(content_count);
    chunk.m_size = batch_size;
    google_batch_t batch{ std::move(easy) };
    batch.path_set("/drive/v3");
    std::ifstream stream{ lgo::create_ifstream(file->m_path_local) };
    stream.seekg(static_cast<std::uint64_t>(file->m_chunk_cover_progress));
    for (std::uint64_t i{ 0 }; i < content_count; ++i)
    {
        std::uint64_t size{};
        if (i == content_count - 1)
        {
            size = batch_size - (content_count - 1) * content_size_max_base64;
        }
        else
        {
            size = content_size_max_base64;
        }
        blob_str_opt_t blob
        {
            .m_id{ chunk.m_content_id[i] },
            .m_name
            {
                nlohmann::json
                {
                    { "parent", chunk.m_id }
                }
                .dump()
            }
        };
        std::string content( size, 0 );
        stream.read(content.data(), size);
        blob.m_content = base64url_encode(content);
        blob_update(batch, blob);
    }
    batch.method_post();
    //Get the key to `m_state->m_end` first before `batch` is moved.
    CURL* key{ batch.interface_get() };
    multi.add(std::move(batch));

    file->m_task->m_state->m_end.emplace
    (
        key,
        std::bind(upload_chunk_transfer_end, std::placeholders::_1, file, file->m_chunk_arr.size() - 1)
    );
    file->m_chunk_cover_progress += batch_size;
    return;
}

auto upload_chunk_transfer_end(std::unique_ptr<curl_easy_t>&, upload_file_t* file, std::size_t chunk_i) -> void
{
    upload_chunk_t& chunk{ file->m_chunk_arr[chunk_i] };
    chunk.m_transfer_end = true;
    upload_task_t* task{ file->m_task };
    while
    (
        file->m_chunk_transfer_end_progress < file->m_chunk_arr.size() &&
        file->m_chunk_arr[file->m_chunk_transfer_end_progress].m_transfer_end
    )
    {
        ++file->m_chunk_transfer_end_progress;
    }
    task->m_progress += chunk.m_size;
    task->m_progress_text->set_text
    (
        task->m_type + "：" +
        std::to_string(task->m_progress) + "B/" +
        std::to_string(task->m_size) + "B " +
        task->m_name,
        false
    );
    upload_complete(file);
    return;
}

auto upload_folder_load_begin
(
    curl_multi_t& multi,
    curl_easy_t&& easy,
    blob_queue_t* blob_queue,
    upload_folder_t* folder
)
-> void
{
	blob_opt_t blob
    {
        .m_id{ folder->m_next_id }
    };
    blob.m_content = nlohmann::json::array();
    folder->m_next_id = "null";
    while (folder->m_folder_load_begin_progress < folder->m_entity_arr.size())
    {
        std::size_t element_entity_i{ folder->m_entity_arr[folder->m_folder_load_begin_progress] };
        upload_entity_t* element_entity{ folder->m_task->m_entity_arr[element_entity_i].get() };
        if (element_entity->m_type == "file")
        {
            upload_file_t* element_file{ dynamic_cast<upload_file_t*>(element_entity) };
            blob.m_content.value().push_back
            (
                nlohmann::json
                {
                    { "id", element_file->m_main_id },
                    { "type", "file" },
                    { "name", element_file->m_name },
                    { "size", element_file->m_size }
                }
            );
        }
        else if (element_entity->m_type == "folder")
        {
            upload_folder_t* element_folder{ dynamic_cast<upload_folder_t*>(element_entity) };
            blob.m_content.value().push_back
            (
                nlohmann::json
                {
                    { "id", element_folder->m_main_id },
                    { "type", "folder" },
                    { "name", element_folder->m_name }
                }
            );
        }
        else
        {
            throw;
        }
        if (blob.m_content.value().size() == 100 || blob.m_content.value().dump().size() > content_size_max)
        {
            blob.m_content.value().erase(blob.m_content.value().end() - 1);
            folder->m_next_id = blob_queue->get();
            break;
        }
        ++folder->m_folder_load_begin_progress;
    }
    //Modify `m_name` after `m_content` here because it needs `folder->m_next_id`,
    // which get updated during the evaluation of `m_content`.
	blob.m_name =
		nlohmann::json
		{
			{ "name", folder->m_name },
			{ "parent", folder->m_parent->m_main_id },
			{ "next", folder->m_next_id }
		};
    google_easy_t easy_google{ std::move(easy) };
    easy_google.path_set("/drive/v3");
	blob_update(easy_google, blob);
    //Get the key to `m_state->m_end` first before `easy_google` is moved.
    CURL* key{ easy_google.interface_get() };
    multi.add(std::move(easy_google));
    
    if (folder->m_folder_load_begin_progress == folder->m_entity_arr.size())
    {
        ++folder->m_task->m_begin_progress;
    }
    folder->m_task->m_state->m_end.emplace
    (
        key,
        std::bind
        (
            upload_folder_load_end,
            std::placeholders::_1,
            folder,
            folder->m_folder_load_begin_progress - blob.m_content.value().size(),
            blob.m_content.value().size()
        )
    );
    return;
}

auto upload_folder_load_end
(
    std::unique_ptr<curl_easy_t>&,
    upload_folder_t* folder,
    std::uint64_t begin_i,
    std::uint64_t entity_count
)
-> void
{
    for (std::uint64_t i{ 0 }; i < entity_count; ++i)
    {
        folder->m_entity_load_end[begin_i + i] = true;
    }
    while
    (
        folder->m_folder_load_end_progress < folder->m_entity_arr.size() &&
        folder->m_entity_load_end[folder->m_folder_load_end_progress]
    )
    {
        ++folder->m_folder_load_end_progress;
    }
    if (begin_i + entity_count == folder->m_entity_arr.size())
    {
        folder->m_load_end = true;
    }
    upload_complete(folder);
    return;
}

//Update the idle status of the file and its task after beginning a load or transfer.
//This function only moves the `m_idle_first` pointer forward.
//Moving it backward is the task of the `_end` functions.
auto download_idle_update(std::vector<download_file_t>::iterator file_i) -> void
{
    download_task_t* task{ file_i->m_task };
    while (file_i != task->m_file_arr.end())
    {
        if
        (
            (!file_i->m_file_loading && file_i->m_next_id != "null") ||
            file_i->m_chunk_load_begin_progress < file_i->m_chunk_arr.size() ||
            file_i->m_chunk_transfer_begin_progress < file_i->m_chunk_load_end_progress
        )
        {
            file_i->m_idle = true;
            file_i->m_task->m_idle_first = file_i;
            return;
        }
        file_i->m_idle = false;
        ++file_i;
    }
    task->m_idle_first = file_i;
    return;
}

auto download_file_load_begin
(
    curl_multi_t& multi,
    curl_easy_t&& easy,
    std::vector<download_file_t>::iterator file_i
)
-> void
{
    google_easy_t easy_google{ std::move(easy) };
    blob_get(easy_google, file_i->m_next_id, "name,description");
    //Get the key to `m_state->m_end` first before `easy_google` is moved.
    CURL* key{ easy_google.interface_get() };
    multi.add(std::move(easy_google));

    file_i->m_file_loading = true;
    file_i->m_task->m_state->m_end.emplace
    (
        key,
        std::bind(download_file_load_end, std::placeholders::_1, file_i)
    );
    download_idle_update(file_i);
    return;
}

auto download_file_load_end(std::unique_ptr<curl_easy_t>& easy, std::vector<download_file_t>::iterator file_i) -> void
{
    nlohmann::json payload = nlohmann::json::parse(easy->payload());
    nlohmann::json blob_name = nlohmann::json::parse(payload["name"].get<std::string>());
    nlohmann::json blob_content = nlohmann::json::parse(payload["description"].get<std::string>());
    file_i->m_next_id = blob_name["next"];
    for (const auto& i : blob_content)
    {
        file_i->m_chunk_arr.push_back
        (
            download_chunk_t
            {
                .m_file{ &*file_i },
                .m_id{ i.get<std::string>() }
            }
        );
    }
    file_i->m_file_loading = false;

    //Update idle information.
    file_i->m_idle = true;
    if (file_i < file_i->m_task->m_idle_first)
    {
        file_i->m_task->m_idle_first = file_i;
    }
    return;
}

auto download_chunk_load_begin
(
    curl_multi_t& multi,
    curl_easy_t&& easy,
    std::vector<download_file_t>::iterator file_i
)
-> void
{
    google_batch_t batch{ std::move(easy) };
    batch.path_set("/drive/v3");
    std::size_t i_begin{ file_i->m_chunk_load_begin_progress };
    std::size_t i_end{ std::min(i_begin + 100, file_i->m_chunk_arr.size()) };
    for (std::size_t i{ i_begin }; i < i_end; ++i)
    {
        blob_get(batch, file_i->m_chunk_arr[i].m_id, "description");
    }
    batch.method_post();
    //Get the key to `m_state->m_end` first before `batch` is moved.
    CURL* key{ batch.interface_get() };
    multi.add(std::move(batch));

    file_i->m_chunk_load_begin_progress = i_end;
    file_i->m_task->m_state->m_end.emplace
    (
        key,
        std::bind(download_chunk_load_end, std::placeholders::_1, file_i, i_begin)
    );
    download_idle_update(file_i);
    return;
}

auto download_chunk_load_end
(
    std::unique_ptr<curl_easy_t>& easy,
    std::vector<download_file_t>::iterator file_i,
    std::size_t i_begin
)
-> void
{
    auto& batch{ dynamic_cast<google_batch_t&>(*easy) };
    for (std::uint64_t i{ 0 }; i < batch.part_count(); ++i)
    {
        download_chunk_t& chunk{ file_i->m_chunk_arr[i_begin + i] };
        nlohmann::json payload = nlohmann::json::parse(batch.response_get(i).body_get());
        nlohmann::json content_id_arr = nlohmann::json::parse(payload["description"].get<std::string>());
        chunk.m_content_id.resize(content_id_arr.size());
        std::ranges::copy(content_id_arr, chunk.m_content_id.begin());
    }

    //Update m_chunk_load_end_progress.
    //Some previous chunks can be loading while these chunks finish loading,
    // so first check if all of the previous chunks have finished loading (file_i->m_chunk_load_end_progress == i_begin).
    if (file_i->m_chunk_load_end_progress == i_begin)
    {
        file_i->m_chunk_load_end_progress += batch.part_count();
        while
        (
            file_i->m_chunk_load_end_progress < file_i->m_chunk_arr.size() &&
            //Nonempty m_content_id means the ids were loaded.
            !file_i->m_chunk_arr[file_i->m_chunk_load_end_progress].m_content_id.empty()
        )
        {
            ++file_i->m_chunk_load_end_progress;
        }
    }

    //Update idle information.
    file_i->m_idle = true;
    if (file_i < file_i->m_task->m_idle_first)
    {
        file_i->m_task->m_idle_first = file_i;
    }
    return;
}

auto download_chunk_transfer_begin(curl_multi_t& multi, curl_easy_t&& easy, download_chunk_t& chunk) -> void
{
    std::size_t request_count{ chunk.m_content_id.size() };
    google_batch_t batch{ std::move(easy) };
    batch.path_set("/drive/v3");
    for (std::size_t i{ 0 }; i < request_count; ++i)
    {
        blob_get(batch, chunk.m_content_id[i], "description");
    }
    batch.method_post();
    //Get the key to `m_state->m_end` first before `batch` is moved.
    CURL* key{ batch.interface_get() };
    multi.add(std::move(batch));

    ++chunk.m_file->m_chunk_transfer_begin_progress;
    chunk.m_file->m_task->m_state->m_end.emplace
    (
        key,
        std::bind(download_chunk_transfer_end, std::placeholders::_1, chunk.m_file, &chunk - chunk.m_file->m_chunk_arr.data())
    );
    download_idle_update
    (
        chunk.m_file->m_task->m_file_arr.begin() +
        (chunk.m_file - chunk.m_file->m_task->m_file_arr.data())
    );
    return;
}

//We can't use an iterator nor an address for `chunk` because this file may still be loading,
// so m_chunk_arr may be modified, invalidating the iterator.
auto download_chunk_transfer_end(std::unique_ptr<curl_easy_t>& easy, download_file_t* file, std::size_t chunk_i) -> void
{
    google_batch_t& batch{ dynamic_cast<google_batch_t&>(*easy) };
    download_chunk_t& chunk{ file->m_chunk_arr[chunk_i] };
    chunk.m_content.resize(batch.part_count());
    for (std::uint64_t i{ 0 }; i < batch.part_count(); ++i)
    {
        chunk.m_content[i] = nlohmann::json::parse(batch.response_get(i).body_get())["description"];
    }
    if (chunk.m_file->m_filesystem_progress < chunk_i)
    {
        return;
    }

    std::ofstream content_stream{ lgo::create_ofstream_preserve(chunk.m_file->m_path_local) };
    content_stream.seekp(0, std::ios_base::end);
    std::uint64_t filesystem_size{};
    while (true)
    {
        if (chunk.m_file->m_filesystem_progress == chunk.m_file->m_chunk_arr.size())
        {
            break;
        }
        //This chunk is going to be written to local filesystem.
        download_chunk_t& chunk_filesystem{ chunk.m_file->m_chunk_arr[chunk.m_file->m_filesystem_progress] };
        if (chunk_filesystem.m_content.empty())
        {
            break;
        }
        for (std::uint64_t i{ 0 }; i < chunk_filesystem.m_content.size(); ++i)
        {
            chunk_filesystem.m_content[i] = base64url_decode(chunk_filesystem.m_content[i]);
            content_stream.write(chunk_filesystem.m_content[i].data(), chunk_filesystem.m_content[i].size());
            filesystem_size += chunk_filesystem.m_content[i].size();
            chunk_filesystem.m_content[i].clear();
            chunk_filesystem.m_content[i].shrink_to_fit();
        }
        ++chunk_filesystem.m_file->m_filesystem_progress;
    }

    //Update progress.
    download_task_t& task{ *chunk.m_file->m_task };
    task.m_progress += filesystem_size;
    task.m_progress_text->set_text
    (
        task.m_type + "：" +
        std::to_string(task.m_progress) + "B/" +
        std::to_string(task.m_size) + "B " +
        task.m_name,
        false
    );

    //Remove the task from load state if it is finished.
    if (task.m_progress == task.m_size)
    {
        task.m_state->m_progress_column->remove_object(task.m_progress_div);
        task.m_state->m_task_list.erase(task.m_self_i);
    }

    return;
}
