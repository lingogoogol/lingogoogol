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

namespace
{
    auto task_ids(load_task_t* task, blob_queue_t* queue, std::string key, std::size_t count = 1)
        -> std::vector<std::string>
    {
        if (task->m_control->cancelled) throw std::runtime_error{"Task cancelled."};
        if (task->m_session)
        {
            const auto type = key.starts_with("folder") ? "folder" : key.starts_with("file") ? "file" : "";
            return task_reserve(*task,key,count,{},type);
        }
        return task->m_journal ? task->m_journal->allocate(std::move(key),count,
            [queue,task](std::size_t size) { return queue->get(size,&task->m_control->cancelled); }) :
            queue->get(count,&task->m_control->cancelled);
    }

    auto task_progress(load_task_t& task, std::uint64_t completed, std::uint64_t size,
                       const std::string& name) -> void
    {
        if (task.m_progress_bar)
            task.m_progress_bar->set_fraction(size ? double(std::max(task.m_saved_progress, completed)) / size : 0.);
        if (!task.m_progress_text)
            return; // Scheduler-only callers may have no GUI progress row.
        task.m_progress_text->set_text(
            std::format("{}：{}B/{}B {}", task.m_type == "upload" ? "上傳" : "下載",
                        std::max(task.m_saved_progress, completed), size, name), false);
    }
} // namespace

auto finish_task(load_task_t& task, task_result_t result, std::string text) -> void
{
    if (task.m_result != task_result_t::active) return;
    if (result != task_result_t::completed) task.m_control->cancel();
    // Sign-out interrupts work like process shutdown: keep its last durable
    // checkpoint, including the user's explicit pause state, for next login.
    if (task.m_journal && result != task_result_t::signed_out) task.m_journal->finish();
    if (result == task_result_t::signed_out) task.m_dismissed = true;
    task.m_result = result == task_result_t::cancelled ? task_result_t::cancelling : result;
    if (result == task_result_t::cancelled && text.starts_with("已取消："))
        text.replace(0, std::string_view{"已取消："}.size(), "取消中：");
    if (task.m_progress_text) task.m_progress_text->set_text(std::move(text), false);
    if (task.m_progress_bar && result == task_result_t::completed)
        task.m_progress_bar->set_fraction(1.);
    for (const auto& button : {task.m_pause_button, task.m_resume_button, task.m_retry_button, task.m_cancel_button})
        if (button) button->hide();
    if (task.m_action_separator) task.m_action_separator->hide();
    if (task.m_confirm_button && task.m_result != task_result_t::cancelling)
        task.m_confirm_button->show();
    auto& state{*task.m_state};
    state.m_finished_tasks.splice(state.m_finished_tasks.end(), state.m_task_list, task.m_self_i);
}

auto collect_finished_tasks(load_state_t& state, curl_multi_worker_t& multi) -> void
{
    using namespace std::chrono_literals;
    for (auto entry{state.m_finished_tasks.begin()}; entry != state.m_finished_tasks.end();)
    {
        auto& task{**entry};
        if (task.m_result != task_result_t::cancelling && !task.m_dismissed)
        {
            ++entry;
            continue;
        }
        // Upload callbacks contain raw entity pointers. Removing a row must not
        // free their owner before the worker, parked callbacks and futures drain.
        const auto ready = [](const auto& future)
            { return !future.valid() || future.wait_for(0s) == std::future_status::ready; };
        bool busy{multi.pending(task.m_control)};
        if (const auto upload{std::dynamic_pointer_cast<upload_task_t>(*entry)})
            busy = busy || !ready(upload->m_prepared) || !ready(upload->m_linked);
        if (const auto download{std::dynamic_pointer_cast<download_task_t>(*entry)})
            busy = busy || !ready(download->m_prepared);
        if (const auto operation{std::dynamic_pointer_cast<directory_task_t>(*entry)})
            busy = busy || !ready(operation->m_operation);
        std::erase_if(state.m_parked, [&](const auto& parked)
            { return parked.request->m_control == task.m_control; });
        if (task.m_result == task_result_t::cancelling && !busy)
        {
            task.m_result = task_result_t::cancelled;
            if (task.m_progress_text)
                task.m_progress_text->set_text("已取消：" + task.m_label, false);
            if (task.m_confirm_button) task.m_confirm_button->show();
        }
        if (task.m_dismissed && task.m_result != task_result_t::cancelling)
        {
            if (!task.m_row_removed)
            {
                if (state.m_progress_column) state.m_progress_column->remove_object(task.m_progress_div);
                task.m_row_removed = true;
            }
            if (!busy) { entry = state.m_finished_tasks.erase(entry); continue; }
        }
        ++entry;
    }
}

auto download_fail(download_task_t& task, const std::exception& error) -> void
{
    if (!task.m_error.empty())
        return;
    task.m_error = error.what();
    if (task.m_error.empty()) task.m_error = "未知下載錯誤";
    lgo::log_file("Download failed: " + task.m_name + "\n" + task.m_error + "\n");
    const auto text{"下載失敗：" + task.m_name + "（" + task.m_error + "）"};
    if (task.m_preparation_failed)
    {
        if (task.m_progress_text) task.m_progress_text->set_text(text, false);
    }
    else finish_task(task, task_result_t::failed, text);
}

auto download_on_end(download_task_t& task, CURL* key,
    std::function<void(std::unique_ptr<curl_easy_t>&)> callback) -> void
{
    // Keep task/file storage alive through the last callback, even when completion
    // removes the task from the list. Failed tasks drain existing requests only.
    auto owner{ std::static_pointer_cast<download_task_t>(*task.m_self_i) };
    task.m_state->m_end.emplace(key,
        [owner = std::move(owner), callback = std::move(callback)](std::unique_ptr<curl_easy_t>& easy)
        {
            if (!owner->m_error.empty()) return;
            try { callback(easy); }
            catch (const std::exception& error) { download_fail(*owner, error); }
        });
}

auto list_folder_content(google_easy_t& easy, std::string* pos, storage_session_t* session) -> nlohmann::json
{
    std::string next{ *pos };
	//Use `=` instead of `{}` when initializing `nlohmann::json` from a single `nlohmann::json` expression:
	//brace-initialization `nlohmann::json{ expr }` selects the `std::initializer_list` constructor and
	//wraps `expr` into a 1-element JSON array `[expr]`, causing runtime `type_error.305` on key lookup.
	nlohmann::json entity_arr = nlohmann::json::array();
	while (next != "null")
	{
		if (session) session->bind(easy,next);
        nlohmann::json response_data = blob_get_run(easy, next, "name,description");
		nlohmann::json response_content = nlohmann::json::parse(response_data["description"].get<std::string>());
		if (session) session->observe(next,nlohmann::json::parse(response_data["name"].get<std::string>()),response_content);
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
    file->m_main_id = task_ids(task, blob_queue, "file:" + path_local.string()).front();
    file->m_next_id = file->m_main_id;
    file->m_path_local = path_local.string();
    file->m_size = std::filesystem::file_size(path_local);
    if (task->m_journal)
    {
        const auto key{"source:" + path_local.string()};
        const auto stamp = nlohmann::json{
            {"size", file->m_size},
            {"mtime", std::filesystem::last_write_time(path_local).time_since_epoch().count()}};
        const auto saved = task->m_journal->field(key);
        if (saved && *saved != stamp)
            throw std::runtime_error{"Upload source changed since the saved task: " + path_local.string()};
        task->m_journal->set(key, stamp);
    }
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
    folder->m_main_id = task_ids(task, blob_queue, "folder:" + path_local.string()).front();
    folder->m_next_id = folder->m_main_id;
	folder->m_self_i = task->m_entity_arr.size();
    task->m_entity_arr.emplace_back(folder);
    std::vector<std::filesystem::path> children;
    for (const auto& child : std::filesystem::directory_iterator{path_local})
        children.push_back(child.path());
    std::ranges::sort(children);
    if (task->m_journal)
    {
        std::vector<std::string> names;
        for (const auto& child : children)
            names.push_back(child.filename().string());
        const auto key{"children:" + path_local.string()};
        const auto saved = task->m_journal->field(key);
        if (saved && *saved != names)
            throw std::runtime_error{"Upload directory changed since the saved task."};
        task->m_journal->set(key, names);
    }
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
    file.m_source_id=pos;
    const bool restored=download_progress_restore(file);
    if (!restored)
    {
        if (task->m_session) task->m_session->bind(*easy,pos);
        file.m_size = nlohmann::json::parse(blob_get_name_run(*easy, pos))["size"];
        file.m_next_id = pos;
    }
    if (restored && file.m_complete)
    {
        if (std::filesystem::file_size(path_local)!=file.m_size)
            throw std::runtime_error{"Completed download size differs from its checkpoint."};
    }
    else if (file.m_written)
    {
        if (std::filesystem::file_size(path_local) < file.m_written)
            throw std::runtime_error{"Saved download is shorter than its checkpoint."};
        std::filesystem::resize_file(path_local, file.m_written);
    }
    else
    {
        auto output{lgo::create_ofstream_destroy(path_local.string())};
        if (!output)
            throw std::runtime_error{"Cannot create download file: " + path_local.string()};
    }
    task->m_size += file.m_size;
    task->m_progress += file.m_written;
    task->m_file_arr.emplace_back(std::move(file));
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
    // json's initializer_list constructor would wrap the returned array in another array.
    nlohmann::json obj_list = list_folder_content(*easy, &pos, task->m_session.get());
	std::filesystem::create_directories(path_local);
    task->m_directories.push_back(path_local.string());
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
    nlohmann::json entity,
    storage_session_t* session
)
-> void
{
	nlohmann::json folder{};
	nlohmann::json folder_name{};
	while (true)
	{
        if (session) session->bind(easy,pos);
        folder = blob_get_run(easy, pos, "name,description");
        folder_name = nlohmann::json::parse(folder["name"].get<std::string>());
        const auto existing = nlohmann::json::parse(folder["description"].get<std::string>());
        if (session) session->observe(pos,folder_name,existing);
        for (const auto& entry : existing)
            if (entry.at("id") == entity.at("id"))
                return;
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
        std::string data_id;
        if (session)
        {
            const auto owner = session->choose(1);
            easy.m_account_context = session->accounts.at(owner);
            auto name = folder_name;
            name["next"] = "null";
            name["next_account"] = "null";
            data_id = blob_upload_run(easy,name.dump(),array.dump());
            session->remember(data_id,owner,"folder");
            folder_name["next_account"] = owner;
            session->bind(easy,pos);
        }
        else data_id = folder_data_add(easy,pos,folder_name["name"],folder_name["parent"],array);
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
            { "parent_account", "null" },
            { "next_account", "null" },
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
auto transfer_begin(load_state_t* state, curl_multi_worker_t& multi, curl_easy_t&& easy, blob_queue_t* blob_queue) -> bool
{
    using namespace std::chrono_literals;

    for (auto entry{state->m_parked.begin()}; entry != state->m_parked.end();)
    {
        auto& request{entry->request};
        const auto owner{std::ranges::find_if(state->m_task_list,
            [&](const auto& task) { return task->m_control == request->m_control; })};
        const auto download{owner == state->m_task_list.end() ? nullptr :
                            std::dynamic_pointer_cast<download_task_t>(*owner)};
        if (request->cancelled() || (download && !download->m_error.empty()))
        {
            // Failed downloads drain old callbacks without scheduling more I/O.
            entry = state->m_parked.erase(entry);
            continue;
        }
        if (request->m_control && (request->m_control->user_paused || request->m_control->paused))
        {
            ++entry;
            continue;
        }
        // A completed result must survive a manual network retry unchanged.
        if (request->m_suspended && !request->runnable())
        {
            ++entry;
            continue;
        }
        auto parked{std::move(*entry)};
        state->m_parked.erase(entry);
        if (!parked.request->m_suspended)
        {
            // A fully successful response raced with Pause. Apply it once,
            // after Continue, without issuing the remote mutation again.
            parked.complete(parked.request);
            return false;
        }
        auto* key{easy.take_transport()};
        parked.request->bind_transport(key);
        state->m_end.emplace(key, std::move(parked.complete));
        multi.add(std::move(parked.request));
        return true;
    }

    // Rotate runnable jobs so a large copy/delete cannot monopolize all newly
    // available slots while an upload or download is also waiting for one.
    auto entry{std::ranges::find(state->m_task_list,state->m_next_transfer.lock())};
    if (entry==state->m_task_list.end()) entry=state->m_task_list.begin();
    for (auto remaining{state->m_task_list.size()}; remaining; --remaining)
    {
        auto task_ptr{*entry};
        if (++entry==state->m_task_list.end()) entry=state->m_task_list.begin();
        state->m_next_transfer=*entry;
        if (task_ptr->m_control->cancelled || task_ptr->m_control->paused || task_ptr->m_control->user_paused ||
            (task_ptr->m_session && task_ptr->m_session->clearing))
            continue;
        if (task_ptr->m_session)
        {
            blob_queue = task_ptr->m_session->queue.get();
            easy.m_account_context = task_ptr->m_session->account;
        }
        else
            easy.m_account_context.reset();
        easy.control_set(task_ptr->m_control);
        easy.m_journal = task_ptr->m_journal;
        if (task_ptr->m_type == "directory")
        {
            auto task{std::static_pointer_cast<directory_task_t>(task_ptr)};
            const auto total{task->m_total.load()}, completed{task->m_completed.load()};
            if (task->m_progress_bar)
                task->m_progress_bar->set_fraction(total ? double(completed) / total : 0.);
            if (task->m_progress_text)
                task->m_progress_text->set_text(task->m_label + (total ? std::format("（{}/{}）", completed, total) : "（等待中）"), false);
            if (task->m_operation.wait_for(0s) != std::future_status::ready)
            {
                if (directory_transfer_begin(*task, multi, std::move(easy))) return true;
                continue;
            }
            try
            {
                task->m_operation.get();
                if (state->m_on_directory_changed && task->m_session == state->m_session)
                    state->m_on_directory_changed(*task);
                finish_task(*task, task_result_t::completed, "已完成：" + task->m_label);
            }
            catch (const std::exception& error)
            {
                lgo::log_file("Directory operation failed: " + std::string{error.what()} + "\n");
                finish_task(*task, task_result_t::failed, "操作失敗：" + task->m_label + "（" + error.what() + "）");
            }
            return false;
        }
        if (task_ptr->m_type == "upload")
        {
            auto upload_task_ptr{ std::dynamic_pointer_cast<upload_task_t>(task_ptr) };
            if (upload_task_ptr->m_linked.valid())
            {
                if (upload_task_ptr->m_linked.wait_for(0s) == std::future_status::ready)
                {
                    // Keep the progress entry until publication succeeds, and propagate errors.
                    try
                    {
                        upload_task_ptr->m_linked.get();
                    }
                    catch (const std::exception& error)
                    {
                        upload_task_ptr->m_control->fail(error.what());
                        continue;
                    }
                    if (state->m_on_upload_linked &&
                        (!upload_task_ptr->m_session || !state->m_session ||
                         upload_task_ptr->m_session->storage_id == state->m_session->storage_id))
                    {
                        state->m_on_upload_linked(upload_task_ptr->m_pos.m_main_id);
                    }
                    finish_task(*upload_task_ptr, task_result_t::completed, "已完成：上傳 " + upload_task_ptr->m_name);
                    return false;
                }
                continue;
            }
            if (upload_task_ptr->m_prepared.valid())
            {
                if (upload_task_ptr->m_prepared.wait_for(0s) != std::future_status::ready)
                {
                    continue;
                }
                // Consume the future so preparation failures cannot be mistaken for success.
                try
                {
                    upload_task_ptr->m_prepared.get();
                }
                catch (const std::exception& error)
                {
                    upload_task_ptr->m_preparation_failed = true;
                    upload_task_ptr->m_control->fail(error.what());
                    continue;
                }
                // Preparation has computed the complete file/folder size. Show
                // 0B/size now, before waiting for the first successful chunk.
                task_progress(*upload_task_ptr, upload_task_ptr->m_progress,
                              upload_task_ptr->m_size, upload_task_ptr->m_name);
                if (upload_task_ptr->m_end_progress==upload_task_ptr->m_entity_arr.end())
                {
                    upload_complete(upload_task_ptr->m_entity_arr.front().get());
                    continue;
                }
            }
            // Downloads consume no reserved blobs. Apply the upload reserve only here.

            auto entity_i{ upload_task_ptr->m_begin_progress };
            if (entity_i == upload_task_ptr->m_entity_arr.end())
            {
                continue;
            }
            if (upload_finished(**entity_i))
            { ++upload_task_ptr->m_begin_progress; continue; }
            // Preflight only ID allocation on a worker. The actual request is
            // still dispatched through the shared foreground pool on this thread.
            // Journal allocation is idempotent, so the dispatch below reuses the
            // same IDs without waiting on a depleted per-account queue.
            std::function<void()> allocate;
            auto* task=upload_task_ptr.get();
            if (auto* file=dynamic_cast<upload_file_t*>(entity_i->get()))
            {
                const auto pending=std::ranges::find_if(file->m_chunk_arr,[](const auto& c)
                    { return !c.m_transfer_end && !c.m_transfer_started; });
                if (pending==file->m_chunk_arr.end() && file->m_chunk_cover_progress<file->m_size)
                {
                    const auto group="chunk-group:"+file->m_main_id+":"+std::to_string(file->m_chunk_arr.size()/100);
                    const auto key="chunk:"+file->m_main_id+":"+std::to_string(file->m_chunk_cover_progress);
                    const auto size=std::min(file->m_size-file->m_chunk_cover_progress,content_size_max_base64*100);
                    const auto count=(size+content_size_max_base64-1)/content_size_max_base64;
                    allocate=[task,group,key,count]
                    {
                        const auto owner=task_owner(*task,group,100);
                        const auto chunk=task_reserve(*task,key,1,owner,"chunk").front();
                        task_reserve(*task,"content:"+chunk,count,{},"data");
                    };
                }
                else if (file->m_chunk_cover_progress==file->m_size &&
                         std::ranges::none_of(file->m_chunk_arr,[](const auto& c) { return !c.m_load_end && !c.m_load_started; }) &&
                         file->m_chunk_arr.size()-file->m_file_load_begin_progress>100)
                {
                    const auto key="file-page:"+file->m_next_id;
                    allocate=[task,key] { task_reserve(*task,key,1,{},"file"); };
                }
            }
            else if (auto* folder=dynamic_cast<upload_folder_t*>(entity_i->get()); folder && folder->m_next_id!="null")
            {
                // A final page's unused reserve remains an ordinary unlinked
                // blob; preallocating avoids blocking if its size limit is hit.
                const auto key="folder-page:"+folder->m_next_id;
                allocate=[task,key] { task_reserve(*task,key,1,{},"folder"); };
            }
            if (allocate)
            {
                if (!task->m_allocation.valid()) task->m_allocation=std::async(std::launch::async,std::move(allocate));
                if (task->m_allocation.wait_for(0s)!=std::future_status::ready) continue;
                try { task->m_allocation.get(); }
                catch (const std::exception& error) { task->m_control->fail(error.what()); continue; }
            }
            if ((*entity_i)->m_type == "file")
            {
                auto file_ptr{ dynamic_cast<upload_file_t*>(entity_i->get()) };
                if (file_ptr->m_chunk_cover_progress < file_ptr->m_size ||
                    std::ranges::any_of(file_ptr->m_chunk_arr,[](const auto& c) { return !c.m_transfer_end && !c.m_transfer_started; }))
                {
                    upload_chunk_transfer_begin(multi, std::move(easy), blob_queue, file_ptr);
                }
                else if (std::ranges::any_of(file_ptr->m_chunk_arr,[](const auto& c) { return !c.m_load_end && !c.m_load_started; }))
                {
                    upload_chunk_load_begin(multi, std::move(easy), file_ptr);
                }
                else if (file_ptr->m_next_id != "null" ||
                    std::ranges::any_of(file_ptr->m_pages,[](const auto& p) { return !p.complete && !p.started; }))
                {
                    upload_file_load_begin(multi, std::move(easy), blob_queue, file_ptr);
                }
                else
                {
                    ++upload_task_ptr->m_begin_progress;
                    continue;
                }
            }
            else if ((*entity_i)->m_type == "folder")
            {
                auto folder_ptr{ dynamic_cast<upload_folder_t*>(entity_i->get()) };
                if (folder_ptr->m_next_id!="null" ||
                    std::ranges::any_of(folder_ptr->m_pages,[](const auto& p) { return !p.complete && !p.started; }))
                    upload_folder_load_begin(multi, std::move(easy), blob_queue, folder_ptr);
                else { ++upload_task_ptr->m_begin_progress; continue; }
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
            if (download_task_ptr->m_prepared.valid())
            {
                if (download_task_ptr->m_prepared.wait_for(0s) != std::future_status::ready)
                {
                    continue;
                }
                // A ready future can contain a preparation error. Consume it before
                // inspecting containers/iterators that the worker may not have initialized.
                if (!download_task_ptr->m_error.empty()) continue;
                try { download_task_ptr->m_prepared.get(); }
                catch (const std::exception& error)
                {
                    download_task_ptr->m_preparation_failed = true;
                    download_task_ptr->m_control->fail(error.what());
                    download_fail(*download_task_ptr, error);
                    continue;
                }
                download_task_ptr->m_idle_first = download_task_ptr->m_file_arr.begin();
                download_task_ptr->m_files_remaining=std::ranges::count_if(download_task_ptr->m_file_arr,[](const auto& f) { return !f.m_complete; });
                for (auto& f:download_task_ptr->m_file_arr)
                    for (auto& c:f.m_chunk_arr) c.m_file=&f;
                task_progress(*download_task_ptr, download_task_ptr->m_progress,
                              download_task_ptr->m_size, download_task_ptr->m_name);
                if (!download_task_ptr->m_files_remaining)
                {
                    // An empty directory tree is complete once its directories exist.
                    finish_task(*download_task_ptr, task_result_t::completed, "已完成：下載 " + download_task_ptr->m_name);
                    return false;
                }
                for (auto& f:download_task_ptr->m_file_arr)
                    if (!f.m_complete)
                    {
                        download_write_ready(f);
                        download_complete(f);
                    }
                if (download_task_ptr->m_result!=task_result_t::active) return false;
                download_idle_update(download_task_ptr->m_file_arr.begin());
            }
            if (!download_task_ptr->m_error.empty()) continue;
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
                // A response can complete out of order while earlier requests
                // remain in flight. Waiting is a normal scheduler state.
                download_idle_update(file_i);
                continue;
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
    auto* key{easy->interface_get()};
    auto callback{state->m_end.extract(key)};
    if (callback.empty()) throw std::logic_error{"Missing transfer completion callback."};
    if (!easy->cancelled() && (easy->m_suspended || (easy->m_control && easy->m_control->user_paused)))
    {
        // Transfer the detached physical handle back to the foreground pool. The
        // parked object retains batch state, account, IDs, retries and callback.
        auto idle{std::make_unique<google_easy_t>(curl_easy_t{easy->take_transport()})};
        state->m_parked.push_back({std::move(easy), std::move(callback.mapped())});
        easy = std::move(idle);
    }
    else if (!easy->cancelled()) callback.mapped()(easy);
	return;
}

//Publish the root only after all content and metadata requests have succeeded.
auto upload_complete(upload_entity_t* entity) -> void
{
    upload_task_t* task{ entity->m_task };

    //Update task progress.
    if (task->m_end_progress!=task->m_entity_arr.end() && entity != task->m_end_progress->get())
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

    task_progress_save(*task);
    if (task->m_end_progress == task->m_entity_arr.end())
    {
        auto root{ task->m_entity_arr.front().get() };
        nlohmann::json link
        {
            { "type", root->m_type },
            { "name", root->m_name }
        };
        if (root->m_type == "file")
        {
            auto file{ dynamic_cast<upload_file_t*>(root) };
            link["id"] = file->m_main_id;
            link["size"] = file->m_size;
        }
        else
        {
            link["id"] = dynamic_cast<upload_folder_t*>(root)->m_main_id;
        }
        if (task->m_session) link["account"] = task->m_session->owner(link.at("id"));
        task->m_progress_text->set_text("上傳：更新目錄 " + task->m_name, false);
        //Each worker owns its easy handle. Waiting for the previous publication prevents
        //concurrent folder_add read/modify/write operations from overwriting each other.
        //Copy the inputs so the worker never accesses task or GUI state.
        task->m_linked =
            std::async(std::launch::async,
                       [previous = task->m_state->m_upload_linked, pos = task->m_pos.m_main_id,
                        link = std::move(link), session = task->m_session, control = task->m_control]
                       {
                           if (previous.valid())
                           {
                               previous.wait();
                           }
                           google_easy_t request;
                           if (session)
                               request.m_account_context = session->account;
                           request.control_set(control);
                           // Publication rereads the current directory. Cached GETs here could
                           // overwrite links published by other tasks after a restored upload.
                           folder_add(request, pos, link, session.get());
                       })
                .share();
        task->m_state->m_upload_linked = task->m_linked;
    }
    return;
}

auto upload_file_load_begin
(
    curl_multi_worker_t& multi,
    curl_easy_t&& easy,
    blob_queue_t* blob_queue,
    upload_file_t* file
)
-> void
{
	auto pending=std::ranges::find_if(file->m_pages,[](const auto& p) { return !p.complete && !p.started; });
    const bool replay=pending!=file->m_pages.end();
    const auto begin=replay ? pending->begin : file->m_file_load_begin_progress;
	blob_opt_t blob
    {
		.m_id{ replay ? pending->id : file->m_next_id }
    };
    std::size_t chunk_count{};
    std::string next;
    if (replay) { chunk_count=pending->count; next=pending->next; }
    else if (file->m_chunk_arr.size() - begin > 100)
    {
        next = task_ids(file->m_task, blob_queue, "file-page:" + blob.m_id).front();
        chunk_count = 100;
    }
    else
    {
        next = "null";
        chunk_count = file->m_chunk_arr.size() - begin;
    }
	blob.m_name =
		nlohmann::json
		{
			{ "name", std::filesystem::path{ file->m_path_local }.filename().string() },
			{ "size", file->m_size },
			{ "parent", file->m_parent->m_main_id },
			{ "next", next }
		};
	blob.m_content = nlohmann::json::array();
    for (std::size_t i{ 0 }; i < chunk_count; ++i)
    {
        blob.m_content->push_back(file->m_chunk_arr[begin + i].m_id);
    }
    if (auto session = file->m_task->m_session)
    {
        (*blob.m_name)["parent_account"] = session->owner(file->m_parent->m_main_id);
        (*blob.m_name)["next_account"] = session->owner(next);
        (*blob.m_name)["account"] = chunk_count ? session->owner(file->m_chunk_arr[begin].m_id) : "null";
        session->bind(easy,blob.m_id);
    }
    google_easy_t easy_google{ std::move(easy) };
    easy_google.path_set("/drive/v3");
	blob_update(easy_google, blob);
    //Get the key to `m_state->m_end` first before `easy_google` is moved.
    CURL* key{ easy_google.interface_get() };
    if (replay) pending->started=true;
    else
    {
        file->m_pages.push_back({blob.m_id,next,begin,chunk_count,false,true});
        file->m_next_id=next;
        file->m_file_load_begin_progress+=chunk_count;
    }
    upload_progress_save(*file); // Durable page identity before remote mutation.
    multi.add(std::move(easy_google));
    
    if (file->m_next_id=="null" && std::ranges::none_of(file->m_pages,[](const auto& p) { return !p.complete && !p.started; }))
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
            begin,
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
    for (auto& p:file->m_pages) if (p.begin==begin_i) p.complete=true;
    upload_progress_save(*file);
    task_progress_save(*file->m_task);
    upload_complete(file);
    return;
}

auto upload_chunk_load_begin(curl_multi_worker_t& multi, curl_easy_t&& easy, upload_file_t* file) -> void
{
    const auto first=std::ranges::find_if(file->m_chunk_arr,[](const auto& c) { return !c.m_load_end && !c.m_load_started; });
    const auto begin=static_cast<std::size_t>(first-file->m_chunk_arr.begin());
    file->m_task->m_session->bind(easy,first->m_id);
    google_batch_t batch{ std::move(easy) };
    batch.path_set("/drive/v3");
    std::size_t chunk_count{};
    while (begin+chunk_count<file->m_chunk_arr.size() && chunk_count<100-begin%100 &&
           !file->m_chunk_arr[begin+chunk_count].m_load_end && !file->m_chunk_arr[begin+chunk_count].m_load_started)
        ++chunk_count;
    for (std::uint64_t i{ 0 }; i < chunk_count; ++i)
    {
        upload_chunk_t& chunk{ file->m_chunk_arr[begin + i] };
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
                    { "parent", file->m_main_id },
                    { "parent_account", file->m_task->m_session->owner(file->m_main_id) },
                    { "account", file->m_task->m_session->owner(chunk.m_content_id.front()) }
                }
            },
            .m_content{ id_array }
        };
        blob_update(batch, blob);
        chunk.m_load_started=true;
    }
    batch.method_post();
    //Get the key to `m_state->m_end` first before `batch` is moved.
    CURL* key{ batch.interface_get() };

    file->m_task->m_state->m_end.emplace
    (
        key,
        std::bind
        (
            upload_chunk_load_end,
            std::placeholders::_1,
            file,
            begin,
            chunk_count
        )
    );
    file->m_chunk_load_begin_progress = begin+chunk_count;
    multi.add(std::move(batch));
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
    upload_progress_save(*file);
    task_progress_save(*file->m_task);
    upload_complete(file);
    return;
}

auto upload_chunk_transfer_begin
(
    curl_multi_worker_t& multi,
    curl_easy_t&& easy,
    blob_queue_t*,
    upload_file_t* file
)
-> void
{
    auto first=std::ranges::find_if(file->m_chunk_arr,[](const auto& c) { return !c.m_transfer_end && !c.m_transfer_started; });
    const bool existing=first!=file->m_chunk_arr.end();
    const auto index=static_cast<std::size_t>(first-file->m_chunk_arr.begin());
    if (!existing)
    {
        const auto group="chunk-group:"+file->m_main_id+":"+std::to_string(index/100);
        const auto key="chunk:"+file->m_main_id+":"+std::to_string(file->m_chunk_cover_progress);
        const auto owner=task_owner(*file->m_task,group);
        file->m_chunk_arr.push_back({.m_file=file,.m_id=task_reserve(*file->m_task,key,1,owner,"chunk").front()});
    }
    auto& chunk=file->m_chunk_arr[index];
    std::uint64_t batch_size{ existing ? chunk.m_size : std::min(file->m_size - file->m_chunk_cover_progress, content_size_max_base64 * 100) };
    //Divide and round up.
    std::uint64_t content_count{ (batch_size + content_size_max_base64 - 1) / content_size_max_base64 };
    if (chunk.m_content_id.empty()) chunk.m_content_id = task_reserve(*file->m_task,"content:"+chunk.m_id,content_count,{},"data");
    file->m_task->m_session->bind(easy,chunk.m_content_id.front());
    chunk.m_size = batch_size;
    google_batch_t batch{ std::move(easy) };
    batch.path_set("/drive/v3");
    std::ifstream stream{ lgo::create_ifstream(file->m_path_local) };
    stream.seekg(static_cast<std::uint64_t>(index*content_size_max_base64*100));
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
                    { "parent", chunk.m_id },
                    { "parent_account", file->m_task->m_session->owner(chunk.m_id) }
                }
                .dump()
            }
        };
        std::string content( size, 0 );
        stream.read(content.data(), size);
        if (stream.gcount() != static_cast<std::streamsize>(size))
        {
            throw lgo::error_t{ "The upload source could not be read completely: " + file->m_path_local };
        }
        blob.m_content = base64url_encode(content);
        blob_update(batch, blob);
    }
    batch.method_post();
    //Get the key to `m_state->m_end` first before `batch` is moved.
    CURL* key{ batch.interface_get() };
    chunk.m_transfer_started=true;
    if (!existing) file->m_chunk_cover_progress += batch_size;
    upload_progress_save(*file);

    file->m_task->m_state->m_end.emplace
    (
        key,
        std::bind(upload_chunk_transfer_end, std::placeholders::_1, file, index)
    );
    multi.add(std::move(batch));
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
    upload_progress_save(*file);
    task_progress_save(*task);
    task_progress(*task, task->m_progress, task->m_size, task->m_name);
    upload_complete(file);
    return;
}

auto upload_folder_load_begin
(
    curl_multi_worker_t& multi,
    curl_easy_t&& easy,
    blob_queue_t* blob_queue,
    upload_folder_t* folder
)
-> void
{
	auto pending=std::ranges::find_if(folder->m_pages,[](const auto& p) { return !p.complete && !p.started; });
    const bool replay=pending!=folder->m_pages.end();
    const auto begin=replay ? pending->begin : folder->m_folder_load_begin_progress;
    auto cursor=begin;
    const auto end=replay ? begin+pending->count : folder->m_entity_arr.size();
    std::string next=replay ? pending->next : "null";
	blob_opt_t blob
    {
        .m_id{ replay ? pending->id : folder->m_next_id }
    };
    blob.m_content = nlohmann::json::array();
    while (cursor < end)
    {
        std::size_t element_entity_i{ folder->m_entity_arr[cursor] };
        upload_entity_t* element_entity{ folder->m_task->m_entity_arr[element_entity_i].get() };
        if (element_entity->m_type == "file")
        {
            upload_file_t* element_file{ dynamic_cast<upload_file_t*>(element_entity) };
            blob.m_content.value().push_back
            (
                nlohmann::json
                {
                    { "id", element_file->m_main_id },
                    { "account", folder->m_task->m_session->owner(element_file->m_main_id) },
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
                    { "account", folder->m_task->m_session->owner(element_folder->m_main_id) },
                    { "type", "folder" },
                    { "name", element_folder->m_name }
                }
            );
        }
        else
        {
            throw;
        }
        if (!replay && (blob.m_content.value().size() == 100 || blob.m_content.value().dump().size() > content_size_max))
        {
            blob.m_content.value().erase(blob.m_content.value().end() - 1);
            next = task_ids(folder->m_task, blob_queue, "folder-page:" + blob.m_id).front();
            break;
        }
        ++cursor;
    }
    //Modify `m_name` after `m_content` here because it needs `folder->m_next_id`,
    // which get updated during the evaluation of `m_content`.
	blob.m_name =
		nlohmann::json
		{
			{ "name", folder->m_name },
			{ "parent", folder->m_parent->m_main_id },
			{ "next", next }
		};
    if (auto session = folder->m_task->m_session)
    {
        (*blob.m_name)["parent_account"] = session->owner(folder->m_parent->m_main_id);
        (*blob.m_name)["next_account"] = session->owner(next);
        session->bind(easy,blob.m_id);
    }
    google_easy_t easy_google{ std::move(easy) };
    easy_google.path_set("/drive/v3");
	blob_update(easy_google, blob);
    //Get the key to `m_state->m_end` first before `easy_google` is moved.
    CURL* key{ easy_google.interface_get() };
    if (replay) pending->started=true;
    else
    {
        folder->m_pages.push_back({blob.m_id,next,begin,cursor-begin,false,true});
        folder->m_folder_load_begin_progress=cursor;
        folder->m_next_id=next;
    }
    upload_progress_save(*folder);
    multi.add(std::move(easy_google));
    
    if (folder->m_next_id=="null" && std::ranges::none_of(folder->m_pages,[](const auto& p) { return !p.complete && !p.started; }))
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
            begin,
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
    for (auto& p:folder->m_pages) if (p.begin==begin_i) p.complete=true;
    upload_progress_save(*folder);
    task_progress_save(*folder->m_task);
    upload_complete(folder);
    return;
}

// Byte totals alone cannot prove a folder is complete: empty files may still
// have metadata requests in flight. Keep their task alive until every file finishes.
auto download_complete(download_file_t& file) -> void
{
    if (file.m_complete || file.m_file_loading || file.m_next_id != "null" ||
        file.m_filesystem_progress != file.m_chunk_arr.size())
    {
        return;
    }
    if (file.m_written != file.m_size)
        throw std::runtime_error{ "Downloaded size differs from the declared size: " + file.m_path_local };
    file.m_complete = true;
    download_progress_save(file);
    auto& task{ *file.m_task };
    if (--task.m_files_remaining == 0)
    {
        if (task.m_progress != task.m_size)
            throw std::runtime_error{ "Downloaded size differs from the declared size: " + task.m_name };
        finish_task(task, task_result_t::completed, "已完成：下載 " + task.m_name);
    }
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
            !file_i->m_complete && ((!file_i->m_file_loading && file_i->m_next_id != "null") ||
            file_i->m_chunk_load_begin_progress < file_i->m_chunk_arr.size() ||
            file_i->m_chunk_transfer_begin_progress < file_i->m_chunk_load_end_progress)
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
    curl_multi_worker_t& multi,
    curl_easy_t&& easy,
    std::vector<download_file_t>::iterator file_i
)
-> void
{
    if (file_i->m_task->m_session) file_i->m_task->m_session->bind(easy,file_i->m_next_id);
    google_easy_t easy_google{ std::move(easy) };
    blob_get(easy_google, file_i->m_next_id, "name,description");
    //Get the key to `m_state->m_end` first before `easy_google` is moved.
    CURL* key{ easy_google.interface_get() };
    multi.add(std::move(easy_google));

    file_i->m_file_loading = true;
    download_on_end
    (
        *file_i->m_task,
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
    if (!blob_content.is_array() || blob_content.size() > 100)
        throw std::runtime_error{ "File chunk index must be an array of at most 100 IDs." };
    if (file_i->m_task->m_session)
        file_i->m_task->m_session->observe(file_i->m_next_id,blob_name,blob_content);
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
    while (file_i->m_chunk_load_begin_progress<file_i->m_chunk_arr.size() && file_i->m_chunk_arr[file_i->m_chunk_load_begin_progress].m_loaded)
        ++file_i->m_chunk_load_begin_progress;
    file_i->m_chunk_load_end_progress=std::max(file_i->m_chunk_load_end_progress,file_i->m_filesystem_progress);
    file_i->m_chunk_transfer_begin_progress=std::max(file_i->m_chunk_transfer_begin_progress,file_i->m_filesystem_progress);
    file_i->m_file_loading = false;
    download_progress_save(*file_i);

    // An empty file has no chunk work to schedule. Skip it while preserving
    // earlier files whose responses may also have made them runnable.
    download_idle_update(std::min(file_i, file_i->m_task->m_idle_first));
    download_complete(*file_i);
    return;
}

auto download_chunk_load_begin
(
    curl_multi_worker_t& multi,
    curl_easy_t&& easy,
    std::vector<download_file_t>::iterator file_i
)
-> void
{
    if (file_i->m_task->m_session)
        file_i->m_task->m_session->bind(easy,file_i->m_chunk_arr[file_i->m_chunk_load_begin_progress].m_id);
    google_batch_t batch{ std::move(easy) };
    batch.path_set("/drive/v3");
    std::size_t i_begin{ file_i->m_chunk_load_begin_progress };
    std::size_t i_end{ std::min(i_begin + 100, file_i->m_chunk_arr.size()) };
    if (auto session = file_i->m_task->m_session)
        for (auto i=i_begin+1;i<i_end;++i)
            if (file_i->m_chunk_arr[i].m_loaded ||
                session->owner(file_i->m_chunk_arr[i].m_id)!=session->owner(file_i->m_chunk_arr[i_begin].m_id))
            { i_end=i; break; }
    for (auto i=i_begin+1;i<i_end;++i)
        if (file_i->m_chunk_arr[i].m_loaded) { i_end=i; break; }
    for (std::size_t i{ i_begin }; i < i_end; ++i)
    {
        blob_get(batch, file_i->m_chunk_arr[i].m_id, "name,description");
    }
    batch.method_post();
    //Get the key to `m_state->m_end` first before `batch` is moved.
    CURL* key{ batch.interface_get() };

    file_i->m_chunk_load_begin_progress = i_end;
    while (file_i->m_chunk_load_begin_progress<file_i->m_chunk_arr.size() &&
           file_i->m_chunk_arr[file_i->m_chunk_load_begin_progress].m_loaded)
        ++file_i->m_chunk_load_begin_progress;
    download_on_end
    (
        *file_i->m_task,
        key,
        std::bind(download_chunk_load_end, std::placeholders::_1, file_i, i_begin)
    );
    download_idle_update(file_i);
    multi.add(std::move(batch));
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
        if (!content_id_arr.is_array() || content_id_arr.empty() || content_id_arr.size() > 100)
            throw std::runtime_error{ "Chunk content index must contain 1 to 100 IDs." };
        if (file_i->m_task->m_session)
            file_i->m_task->m_session->observe(chunk.m_id,
                nlohmann::json::parse(payload.at("name").get<std::string>()),content_id_arr);
        chunk.m_content_id.resize(content_id_arr.size());
        std::ranges::copy(content_id_arr, chunk.m_content_id.begin());
        chunk.m_loaded = true;
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
            file_i->m_chunk_arr[file_i->m_chunk_load_end_progress].m_loaded
        )
        {
            ++file_i->m_chunk_load_end_progress;
        }
    }
    download_progress_save(*file_i);

    // A later index batch may finish before the first pending batch. It does
    // not make content runnable until the contiguous loaded prefix advances.
    // Recompute from the earliest candidate so other runnable files stay visible.
    download_idle_update(std::min(file_i, file_i->m_task->m_idle_first));
    return;
}

auto download_chunk_transfer_begin(curl_multi_worker_t& multi, curl_easy_t&& easy, download_chunk_t& chunk) -> void
{
    std::size_t request_count{ chunk.m_content_id.size() };
    if (chunk.m_file->m_task->m_session) chunk.m_file->m_task->m_session->bind(easy,chunk.m_content_id.front());
    google_batch_t batch{ std::move(easy) };
    batch.path_set("/drive/v3");
    for (std::size_t i{ 0 }; i < request_count; ++i)
    {
        blob_get(batch, chunk.m_content_id[i], "description");
    }
    batch.method_post();
    //Get the key to `m_state->m_end` first before `batch` is moved.
    CURL* key{ batch.interface_get() };

    ++chunk.m_file->m_chunk_transfer_begin_progress;
    while (chunk.m_file->m_chunk_transfer_begin_progress<chunk.m_file->m_chunk_arr.size() &&
           chunk.m_file->m_chunk_arr[chunk.m_file->m_chunk_transfer_begin_progress].m_received)
        ++chunk.m_file->m_chunk_transfer_begin_progress;
    download_on_end
    (
        *chunk.m_file->m_task,
        key,
        std::bind(download_chunk_transfer_end, std::placeholders::_1, chunk.m_file, &chunk - chunk.m_file->m_chunk_arr.data())
    );
    download_idle_update
    (
        chunk.m_file->m_task->m_file_arr.begin() +
        (chunk.m_file - chunk.m_file->m_task->m_file_arr.data())
    );
    multi.add(std::move(batch));
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
    chunk.m_received=true;
    chunk.m_response_key=easy->checkpoint_key();
    download_progress_save(*file);
    download_write_ready(*file);
    download_complete(*file);
}

auto download_write_ready(download_file_t& target) -> void
{
    auto* file=&target;
    if (file->m_filesystem_progress>=file->m_chunk_arr.size() ||
        file->m_chunk_arr[file->m_filesystem_progress].m_content.empty()) return;

    std::ofstream content_stream{ lgo::create_ofstream_preserve(file->m_path_local) };
    if (!content_stream) throw std::runtime_error{ "Cannot write download file: " + file->m_path_local };
    content_stream.seekp(0, std::ios_base::end);
    std::uint64_t filesystem_size{};
    while (true)
    {
        if (file->m_filesystem_progress == file->m_chunk_arr.size())
        {
            break;
        }
        //This chunk is going to be written to local filesystem.
        download_chunk_t& chunk_filesystem{ file->m_chunk_arr[file->m_filesystem_progress] };
        if (chunk_filesystem.m_content.empty())
        {
            break;
        }
        for (std::uint64_t i{ 0 }; i < chunk_filesystem.m_content.size(); ++i)
        {
            chunk_filesystem.m_content[i] = base64url_decode(chunk_filesystem.m_content[i]);
            content_stream.write(chunk_filesystem.m_content[i].data(),
                                 chunk_filesystem.m_content[i].size());
            filesystem_size += chunk_filesystem.m_content[i].size();
            chunk_filesystem.m_content[i].clear();
            chunk_filesystem.m_content[i].shrink_to_fit();
        }
        ++file->m_filesystem_progress;
    }

    // Finish the write before publishing progress or removing the last task owner.
    content_stream.close();
    if (!content_stream) throw std::runtime_error{ "Cannot finish writing download file: " + file->m_path_local };
    file->m_written += filesystem_size;
    if (file->m_task->m_journal)
    {
        // FlushFileBuffers is also needed for the output, before publishing its
        // prefix in task state. Atomic checkpoint writes alone cannot flush it.
        storage_flush_file(file->m_path_local);
        download_progress_save(*file);
    }
    // Update progress.
    download_task_t& task{*file->m_task};
    task.m_progress += filesystem_size;
    task_progress_save(task);
    task_progress(task, task.m_progress, task.m_size, task.m_name);

    return;
}
