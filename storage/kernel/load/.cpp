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
        if (task->m_control->cancelled)
            throw std::runtime_error{"Task cancelled."};
        if (!task->m_journal)
            return queue->get(count, &task->m_control->cancelled);
        return task->m_journal->allocate(std::move(key), count, [queue, task](std::size_t size)
                                         { return queue->get(size, &task->m_control->cancelled); });
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
        const auto saved = task->m_journal->data();
        if (saved.contains(key) && saved.at(key) != stamp)
            throw std::runtime_error{"Upload source changed since the saved task: " + path_local.string()};
        task->m_journal->set(key, stamp);
    }
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
        const auto saved = task->m_journal->data();
        if (saved.contains(key) && saved.at(key) != names)
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
    if (task->m_journal)
    {
        const auto saved = task->m_journal->data();
        file.m_resume_written = saved.value("written:" + path_local.string(), std::uint64_t{});
    }
    if (file.m_resume_written)
    {
        if (std::filesystem::file_size(path_local) < file.m_resume_written)
            throw std::runtime_error{"Saved download is shorter than its checkpoint."};
        std::filesystem::resize_file(path_local, file.m_resume_written);
    }
    else
    {
        auto output{lgo::create_ofstream_destroy(path_local.string())};
        if (!output)
            throw std::runtime_error{"Cannot create download file: " + path_local.string()};
    }
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
    // json's initializer_list constructor would wrap the returned array in another array.
    nlohmann::json obj_list = list_folder_content(*easy, &pos);
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
        const auto existing = nlohmann::json::parse(folder["description"].get<std::string>());
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
                         upload_task_ptr->m_session->account == state->m_session->account))
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
            }
            // Downloads consume no reserved blobs. Apply the upload reserve only here.
            if (blob_queue->count() <= 10000) continue;
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
                download_task_ptr->m_files_remaining = download_task_ptr->m_file_arr.size();
                task_progress(*download_task_ptr, download_task_ptr->m_progress,
                              download_task_ptr->m_size, download_task_ptr->m_name);
                if (!download_task_ptr->m_files_remaining)
                {
                    // An empty directory tree is complete once its directories exist.
                    finish_task(*download_task_ptr, task_result_t::completed, "已完成：下載 " + download_task_ptr->m_name);
                    return false;
                }
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
        // Transfer the detached physical handle back to the 64-slot pool. The
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
                           folder_add(request, pos, link);
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
	blob_opt_t blob
    {
        .m_id{ file->m_next_id }
    };
    std::size_t chunk_count{};
    if (file->m_chunk_arr.size() - file->m_file_load_begin_progress > 100)
    {
        file->m_next_id = task_ids(file->m_task, blob_queue, "file-page:" + blob.m_id).front();
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

auto upload_chunk_load_begin(curl_multi_worker_t& multi, curl_easy_t&& easy, upload_file_t* file) -> void
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
    curl_multi_worker_t& multi,
    curl_easy_t&& easy,
    blob_queue_t* blob_queue,
    upload_file_t* file
)
-> void
{
    auto& chunk{file->m_chunk_arr.emplace_back(upload_chunk_t{
        .m_file{file},
        .m_id{task_ids(file->m_task, blob_queue,
                       "chunk:" + file->m_main_id + ":" + std::to_string(file->m_chunk_cover_progress))
                  .front()}})};
    std::uint64_t batch_size{ std::min(file->m_size - file->m_chunk_cover_progress, content_size_max_base64 * 100) };
    //Divide and round up.
    std::uint64_t content_count{ (batch_size + content_size_max_base64 - 1) / content_size_max_base64 };
    chunk.m_content_id = task_ids(file->m_task, blob_queue, "content:" + chunk.m_id, content_count);
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
    if (task->m_journal)
        task->m_journal->set("progress", std::max(task->m_saved_progress, task->m_progress));
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
            folder->m_next_id = task_ids(folder->m_task, blob_queue, "folder-page:" + blob.m_id).front();
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
    curl_multi_worker_t& multi,
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
    download_on_end
    (
        *file_i->m_task,
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
        if (!content_id_arr.is_array() || content_id_arr.empty() || content_id_arr.size() > 100)
            throw std::runtime_error{ "Chunk content index must contain 1 to 100 IDs." };
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

    // A later index batch may finish before the first pending batch. It does
    // not make content runnable until the contiguous loaded prefix advances.
    // Recompute from the earliest candidate so other runnable files stay visible.
    download_idle_update(std::min(file_i, file_i->m_task->m_idle_first));
    return;
}

auto download_chunk_transfer_begin(curl_multi_worker_t& multi, curl_easy_t&& easy, download_chunk_t& chunk) -> void
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
    if (!content_stream) throw std::runtime_error{ "Cannot write download file: " + chunk.m_file->m_path_local };
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
            const auto offset{file->m_written + filesystem_size};
            if (offset >= file->m_resume_written)
                content_stream.write(chunk_filesystem.m_content[i].data(),
                                     chunk_filesystem.m_content[i].size());
            else if (offset + chunk_filesystem.m_content[i].size() > file->m_resume_written)
                throw std::runtime_error{"Invalid saved download boundary."};
            filesystem_size += chunk_filesystem.m_content[i].size();
            chunk_filesystem.m_content[i].clear();
            chunk_filesystem.m_content[i].shrink_to_fit();
        }
        ++chunk_filesystem.m_file->m_filesystem_progress;
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
        file->m_task->m_journal->set("written:" + file->m_path_local, file->m_written);
    }
    // Update progress.
    download_task_t& task{*chunk.m_file->m_task};
    task.m_progress += filesystem_size;
    if (task.m_journal)
        task.m_journal->set("progress", std::max(task.m_saved_progress, task.m_progress));
    task_progress(task, task.m_progress, task.m_size, task.m_name);

    download_complete(*file);

    return;
}
