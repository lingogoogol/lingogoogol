module;

#include <nlohmann/json.hpp>

module storage.kernel.GUI;

import std;

import external.curl;
import external.GLFW;

import lgo.dev;
import lgo.io.file;
import lgo.math;
import lgo.GUI;

import storage.dialog;
import storage.http;
import storage.google;
import storage.kernel.load;

auto message_loop(lgo::engine_t* engine, GUI_persistent_t* persistent, curl_multi_worker_t* multi,
                  load_state_t* load_state, blob_queue_t* blob_queue) -> bool
{
    using namespace std::chrono_literals;
    while (!persistent->m_deferred.empty())
    {
        auto action{std::move(persistent->m_deferred.front())};
        persistent->m_deferred.pop_front();
        action();
    }
    if (persistent->m_dialog)
    {
        persistent->m_dialog->resize(engine->get_window_size());
        if (const auto decision{persistent->m_dialog->decision()})
        {
            if (persistent->m_dialog_done(*decision, persistent->m_dialog->values()))
            {
                persistent->m_dialog_done = {};
                persistent->m_dialog.reset();
            }
        }
    }
    if (!persistent->m_dialog && !persistent->m_dialog_queue.empty())
    {
        auto show{std::move(persistent->m_dialog_queue.front())};
        persistent->m_dialog_queue.pop_front();
        show();
    }
    for (auto entry{persistent->m_listings.begin()}; entry != persistent->m_listings.end();)
    {
        if (entry->control->paused && entry->generation == persistent->m_listing_generation)
            persistent->m_directory_status->set_text("目錄更新失敗");
        if (entry->future.wait_for(0s) != std::future_status::ready)
        {
            ++entry;
            continue;
        }
        try
        {
            auto result = entry->future.get();
            if (entry->generation == persistent->m_listing_generation &&
                entry->session == load_state->m_session)
            {
                entry->apply(std::move(result));
                persistent->m_directory_status->set_text("");
            }
        }
        catch (const std::exception& error)
        {
            if (entry->generation == persistent->m_listing_generation)
            {
                persistent->m_directory_status->set_text("目錄更新失敗");
                lgo::log_file(std::string{"Directory refresh failed: "} + error.what() + "\n");
            }
        }
        if (entry->control->paused && entry->generation == persistent->m_listing_generation)
            persistent->m_directory_status->set_text("目錄更新失敗");
        entry = persistent->m_listings.erase(entry);
    }
    if (persistent->m_paste_button)
        persistent->m_paste_button->set_enabled(persistent->m_clipboard &&
            persistent->m_clipboard->session == load_state->m_session &&
            load_state->m_session && !load_state->m_session->clearing && !persistent->m_switching);
    collect_finished_tasks(*load_state, *multi);
    for (auto entry{load_state->m_task_list.begin()}; entry != load_state->m_task_list.end();)
    {
        auto task{*entry++};
        task->m_control->retry_limit = load_state->m_settings.network_retries;
        if (task->m_session)
        {
            task->m_session->queue->control()->retry_limit = load_state->m_settings.network_retries;
            if (task->m_type == "upload" && !task->m_control->cancelled &&
                task->m_session->queue->control()->paused)
                task->m_control->fail(task->m_session->queue->control()->reason());
        }
        if (task->m_control->cancelled)
        {
            finish_task(*task, task_result_t::cancelled, "已取消：" + task->m_label);
            continue;
        }
        else if (task->m_control->paused)
        {
            const auto download{std::dynamic_pointer_cast<download_task_t>(task)};
            if (!download || download->m_error.empty())
                task->m_progress_text->set_text(
                    "網路失敗：" + task->m_label + "（" + task->m_control->reason() + "）", false);
        }
        else if (task->m_control->user_paused)
        {
            task->m_progress_text->set_text("已暫停：" + task->m_paused_text, false);
        }
        const bool directory{task->m_type == "directory"};
        const bool cancelled{task->m_control->cancelled};
        const bool failed{task->m_control->paused};
        const bool paused{task->m_control->user_paused};
        auto visible = [](const auto& button, bool show)
        {
            if (button)
            {
                if (show) button->show();
                else button->hide();
            }
        };
        visible(task->m_retry_button, !cancelled && failed);
        visible(task->m_pause_button, !directory && !cancelled && !failed && !paused);
        visible(task->m_resume_button, !directory && !cancelled && !failed && paused);
        visible(task->m_cancel_button, !directory && !cancelled && (failed || paused));
        visible(task->m_action_separator, !directory && !cancelled && (failed || paused));
    }

    if (load_state->m_session)
        blob_queue = load_state->m_session->queue.get();
    blob_queue->control()->retry_limit = load_state->m_settings.network_retries;
    std::string queue_error;
    try
    {
        blob_queue->check_error();
    }
    catch (const std::exception& error)
    {
        queue_error = error.what();
    }

    auto& easy_idle_arr{persistent->m_easy_idle};
    auto& load_easy_status_map{persistent->m_easy_status};
    if (load_easy_status_map.empty())
    {
        easy_idle_arr.resize(load_easy_count);
        for (std::size_t i{}; i < easy_idle_arr.size(); ++i)
            load_easy_status_map[easy_idle_arr[i].interface_get()].m_text_index = i;
    }

    persistent->m_blob_queue_cycle_rate->set_text("blob_queue_cycle_rate: " +
                                                  std::to_string(blob_queue->cycle_rate()));
    persistent->m_blob_queue_count->set_text(queue_error.empty()
                                                 ? "blob_queue_count: " + std::to_string(blob_queue->count())
                                                 : "預備佇列失敗：" + queue_error);
    persistent->show_diagnostics(glfwGetKey(engine->window_get(), GLFW_KEY_F3));
    for (auto& load_easy_status : load_easy_status_map)
    {
        std::string status_str{};
        if (load_easy_status.second.m_add_time.time_since_epoch() ==
            std::chrono::steady_clock::duration::zero())
        {
            status_str = "idle";
        }
        else
        {
            status_str = std::format("{:.1f}s, {}B",
                std::chrono::duration<double>{std::chrono::steady_clock::now() -
                                              load_easy_status.second.m_add_time}.count(),
                multi->read_progress(load_easy_status.first));
        }
        persistent->m_load_easy_status[load_easy_status.second.m_text_index]->set_text(
            std::format("#{:02} {:#018x}, {}", load_easy_status.second.m_text_index + 1,
                        reinterpret_cast<std::uint64_t>(load_easy_status.first), status_str), false);
    }

    // Submit at most one new request per frame; only the worker performs curl.
    if (!easy_idle_arr.empty())
    {
        CURL* key{easy_idle_arr.back().interface_get()};
        if (transfer_begin(load_state, *multi, std::move(easy_idle_arr.back()), blob_queue))
        {
            load_easy_status_map[key].m_add_time = std::chrono::steady_clock::now();
            easy_idle_arr.pop_back();
        }
    }
    multi->run();
    // Bound completion work so a burst of finished requests still permits input/rendering.
    for (std::size_t completed{}; completed < 8; ++completed)
    {
        if (!multi->ready_nonempty())
        {
            break;
        }
        std::unique_ptr<curl_easy_t> easy{multi->ready_get_unique()};
        transfer_end(load_state, easy);
        load_easy_status_map[easy->interface_get()].m_add_time = {};
        easy_idle_arr.push_back(std::move(dynamic_cast<google_easy_t&>(*easy.get())));
    }
    return engine->message_loop();
}

auto dialog_file(bool folder) -> std::vector<std::string>
{
    // IFileDialog::Show is modal: the constructor returns after selection or cancellation.
    // Waiting for ok() after cancellation would loop forever and ignore window-close requests.
    dialog_t dialog{folder};
    return dialog.result();
}

auto folder_create_callback(lgo::engine_t* engine, GUI_persistent_t* persistent,
                            load_state_t* state, std::string* pos) -> void
{
    const auto session{state->m_session};
    const auto source{*pos};
    persistent->m_dialog_queue.push_back([=]
    {
        persistent->m_dialog = std::make_unique<lgo::modal_dialog_t>(
            engine, state->m_progress_column->get_depth_tracker(), "新增資料夾",
            std::vector<std::string>{},
            std::vector<std::pair<std::string, std::string>>{{"資料夾名稱", ""}});
        persistent->m_dialog_done = [=](bool accepted, const std::vector<std::string>& values)
        {
            if (!accepted) return true;
            try
            {
                directory_validate_name(values.at(0));
                if (state->m_session != session || session->clearing)
                    throw std::runtime_error{"帳戶已變更，請重新開啟對話框"};
                begin_directory_task(state, session, "directory-create", source, {}, {}, values[0]);
                return true;
            }
            catch (const std::exception& error) { persistent->m_dialog->error(error.what()); return false; }
        };
    });
}

namespace
{
    auto task_row(load_state_t* state, const std::shared_ptr<load_task_t>& task, std::string text) -> void
    {
        task->m_state = state;
        task->m_label = text;
        if (task->m_journal)
        {
            const auto saved = task->m_journal->data();
            task->m_saved_progress = saved.value("progress", std::uint64_t{});
            task->m_control->user_paused = saved.value("user_paused", false);
            if (task->m_saved_progress)
                text = "續傳：" + std::to_string(task->m_saved_progress) + "B " + text;
            if (task->m_control->user_paused)
                text = saved.value("paused_text", text);
        }
        // Older task checkpoints used English operation prefixes. Preserve
        // the filename and byte counts while normalizing only those prefixes.
        for (const auto& [old, translated] : {std::pair{"upload：", "上傳："},
                                             std::pair{"download：", "下載："},
                                             std::pair{"upload: ", "上傳："},
                                             std::pair{"download: ", "下載："}})
            if (text.starts_with(old)) text.replace(0, std::string_view{old}.size(), translated);
        task->m_paused_text = text;
        task->m_control->retry_limit = state->m_settings.network_retries;
        task->m_progress_div = state->m_progress_column->add_object<lgo::divy_flex_soft_t>(
            storage_content_depth, state->m_progress_column->space_x(lgo::std_margin), lgo::std_margin,
            lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::top});
        auto row{state->m_progress_column->get_object_shared<lgo::divy_flex_soft_t>(task->m_progress_div)};
        task->m_progress_text = row->add_object_shared<lgo::text_line_scroll_std_t>(
            storage_content_depth, text, lgo::std_white, lgo::size_1D{256}, false);
        task->m_progress_bar = row->add_object_shared<lgo::progress_bar_t>(
            storage_content_depth, lgo::size_2D{256, 10}, lgo::std_green);
        const std::weak_ptr<load_task_t> weak{task};
        // All three primary actions occupy the same slot. Ghost children do
        // not reserve extra width when hidden; the cancel slot stays fixed.
        auto actions{row->add_object_shared<lgo::divx_soft_t>(
            storage_content_depth, lgo::div_create_from_member{}, lgo::size_1D{lgo::font_preference_t::default_get()[0]->leading_get()}, lgo::std_margin,
            row->space_x(lgo::std_margin),
            lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::top})};
        task->m_pause_button = actions->add_object_ghost_shared<progress_button_t>(
            storage_content_depth, [weak]
            {
                if (auto task{weak.lock()})
                    task->pause(true);
            }, "暫停", lgo::std_white, lgo::size_1D{32});
        task->m_resume_button = actions->add_object_ghost_shared<progress_button_t>(
            storage_content_depth, [weak]
            {
                if (auto task{weak.lock()})
                    task->pause(false);
            }, "繼續", lgo::std_white, lgo::size_1D{32});
        task->m_resume_button->hide();
        task->m_retry_button = actions->add_object_shared<progress_button_t>(
            storage_content_depth,
            [weak]
            {
                if (auto task{weak.lock()})
                {
                    task->pause(false);
                    if (task->m_session)
                    {
                        task->m_session->account->retry();
                        task->m_session->queue->retry();
                    }
                    task->m_control->retry();
                    if (task->m_preparation_failed && task->m_retry_prepare)
                    {
                        task->m_preparation_failed = false;
                        task->m_retry_prepare();
                    }
                }
            },
            "重試", lgo::std_white, lgo::size_1D{32});
        task->m_retry_button->hide();
        task->m_action_separator = actions->add_object_shared<lgo::std_liney_t>(
            storage_content_depth, lgo::size_1D{lgo::font_preference_t::default_get()[0]->leading_get()}, lgo::std_white);
        task->m_action_separator->hide();
        task->m_cancel_button = actions->add_object_shared<progress_button_t>(
            storage_content_depth,
            [weak]
            {
                if (auto task{weak.lock()})
                    task->m_control->cancel();
            },
            "取消", lgo::std_white, lgo::size_1D{32});
        task->m_cancel_button->hide();
        task->m_confirm_button = actions->add_object_ghost_shared<progress_button_t>(
            storage_content_depth, [weak]
            {
                // Defer removal until input dispatch returns; destroying the
                // clicked control inside its own callback is unsafe.
                if (auto task{weak.lock()}) task->m_dismissed = true;
            }, "確定", lgo::std_white, lgo::size_1D{32});
        task->m_confirm_button->hide();
        // A div applies leading margins differently to its first ghost child.
        // Anchor all primary actions explicitly to the reserved retry slot.
        task->m_pause_button->set_pos(task->m_retry_button->get_pos());
        task->m_resume_button->set_pos(task->m_retry_button->get_pos());
        task->m_confirm_button->set_pos(task->m_retry_button->get_pos());
        task->m_self_i = state->m_task_list.emplace(state->m_task_list.end(), task);
    }
} // namespace

auto begin_directory_task(load_state_t* state, std::shared_ptr<storage_session_t> session,
                          std::string kind, std::string source, nlohmann::json entity,
                          std::string destination, std::string name,
                          std::shared_ptr<task_journal_t> journal) -> void
{
    if (!session || session->clearing) return;
    if (!journal)
        journal = task_journal_t::create(nlohmann::json{{"kind", kind}, {"path", name}, {"pos", source},
            {"entity", entity}, {"destination", destination}, {"account_path", session->account_path},
            {"account_email", session->address}, {"root_path", session->root_path}, {"root", session->root}});
    auto task{std::make_shared<directory_task_t>()};
    task->m_session = session; task->m_journal = journal; task->m_type = "directory";
    task->m_kind = kind; task->m_name = name; task->m_source = source;
    task->m_destination = destination; task->m_entity = std::move(entity);
    task_row(state, task, (kind == "directory-create" ? "新增資料夾：" :
        kind == "directory-move" ? "剪下貼上：" : kind == "directory-copy" ? "複製：" : "刪除：") + name);
    task->m_pause_button->hide();
    // Share the publication chain: every read/modify/write of a directory is
    // ordered with existing uploads. A worker never reads GUI-owned containers.
    task->m_operation = std::async(std::launch::async,
        [task = task.get(), previous = state->m_upload_linked]
        {
            using namespace std::chrono_literals;
            while (previous.valid() && previous.wait_for(20ms) != std::future_status::ready)
                if (task->m_control->cancelled) throw std::runtime_error{"操作已中止"};
            if (task->m_control->cancelled) throw std::runtime_error{"操作已中止"};
            directory_execute(*task);
        }).share();
    state->m_upload_linked = task->m_operation;
}

auto begin_task(load_state_t* state, std::shared_ptr<storage_session_t> session, std::string kind,
                std::string path, std::string pos, std::shared_ptr<task_journal_t> journal) -> void
{
    if (kind.starts_with("directory-"))
    {
        if (!journal) throw std::invalid_argument{"Directory restoration requires a saved intent."};
        const auto saved = journal->data();
        begin_directory_task(state, session, kind, pos, saved.at("entity"), saved.at("destination"), path, journal);
        return;
    }
    const bool upload{kind.starts_with("upload")};
    const bool folder{kind.ends_with("folder")};
    if (session && session->clearing)
        return;
    if (!journal && session)
        journal = task_journal_t::create(nlohmann::json{{"kind", kind},
                                                        {"path", path},
                                                        {"pos", pos},
                                                        {"account_path", session->account_path},
                                                        {"account_email", session->address},
                                                        {"root_path", session->root_path},
                                                        {"root", session->root}});
    if (upload)
    {
        auto task{std::make_shared<upload_task_t>()};
        task->m_session = session;
        task->m_journal = journal;
        task->m_type = "upload";
        task->m_name = path;
        task->m_pos.m_main_id = pos;
        task_row(state, task, "準備上傳：" + path);
        auto* queue{session ? session->queue.get() : nullptr};
        task->m_retry_prepare = [task = task.get(), queue, folder, path]
        {
            task->m_entity_arr.clear();
            task->m_size = 0;
            task->m_progress = 0;
            task->m_prepared =
                std::async(std::launch::async,
                           [task, queue, folder, path]
                           {
                               if (!queue)
                                   throw std::runtime_error{"No account reserve is available."};
                               if (folder)
                                   folder_upload(queue, task, path, &task->m_pos);
                               else
                                   file_upload(queue, task, path, &task->m_pos);
                               task->m_begin_progress = task->m_entity_arr.begin();
                               task->m_end_progress = task->m_entity_arr.begin();
                           });
        };
        task->m_retry_prepare();
    }
    else
    {
        auto task{std::make_shared<download_task_t>()};
        task->m_session = session;
        task->m_journal = journal;
        task->m_type = "download";
        task->m_name = path;
        task->m_pos = pos;
        task_row(state, task, "準備下載：" + path);
        auto launch = [task, folder, path, pos](bool accepted)
        {
            if (task->m_control->cancelled) return;
            if (!accepted)
            {
                task->m_error = "使用者未同意刪除已有的目的地";
                finish_task(*task, task_result_t::failed, "下載失敗：" + path + "（" + task->m_error + "）");
                return;
            }
            task->m_retry_prepare = [task = task.get(), folder, path, pos]
            {
                task->m_error.clear();
                task->m_file_arr.clear();
                task->m_size = 0;
                task->m_progress = 0;
                task->m_prepared = std::async(std::launch::async,
                                              [task, folder, path, pos]
                                              {
                                                  google_easy_t request;
                                                  if (task->m_session)
                                                      request.m_account_context = task->m_session->account;
                                                  request.control_set(task->m_control);
                                                  request.m_journal = task->m_journal;
                                                  // Only the GUI confirmation path removes an existing
                                                  // destination. Restored jobs retain their durable output
                                                  // prefixes.
                                                  const auto saved = task->m_journal
                                                                         ? task->m_journal->data()
                                                                         : nlohmann::json::object();
                                                  if (!saved.value("destination_ready", false))
                                                  {
                                                      std::filesystem::remove_all(path);
                                                      if (task->m_journal)
                                                          task->m_journal->set("destination_ready", true);
                                                  }
                                                  if (folder)
                                                      folder_download(&request, task, pos, path);
                                                  else
                                                      file_download(&request, task, pos, path);
                                              });
            };
            task->m_retry_prepare();
        };
        const auto saved = journal ? journal->data() : nlohmann::json::object();
        if (saved.value("declined", false))
            launch(false);
        else if (!saved.value("destination_ready", false) &&
                 std::filesystem::symlink_status(path).type() != std::filesystem::file_type::not_found)
        {
            if (state->m_confirm)
                state->m_confirm("目的地已存在，是否刪除後下載？\n" + path, std::move(launch));
            else
                launch(false); // No GUI host is never implicit permission to delete.
        }
        else
            launch(true);
    }
}

auto restore_tasks(load_state_t* state,
                   const std::function<std::shared_ptr<storage_session_t>(const nlohmann::json&)>& session)
    -> void
{
    for (auto& journal : task_journal_t::restore())
    {
        const auto intent = journal->data();
        begin_task(state, session(intent), intent.at("kind"), intent.at("path"), intent.at("pos"), journal);
    }
}

auto file_upload_callback(load_state_t* state, blob_queue_t*, std::string pos) -> void
{
    for (auto& file : dialog_file(false))
        begin_task(state, state->m_session, "upload-file", file, pos);
}
auto folder_upload_callback(load_state_t* state, blob_queue_t*, std::string pos) -> void
{
    for (auto& folder : dialog_file(true))
        begin_task(state, state->m_session, "upload-folder", folder, pos);
}
auto file_download_callback(load_state_t* state, std::string name, std::string pos) -> void
{
    const auto paths{dialog_file(true)};
    if (paths.size() == 1)
        begin_task(state, state->m_session, "download-file",
                   (std::filesystem::path{paths.front()} / name).string(), pos);
}
auto folder_download_callback(load_state_t* state, std::string name, std::string pos) -> void
{
    const auto paths{dialog_file(true)};
    if (paths.size() == 1)
        begin_task(state, state->m_session, "download-folder",
                   (std::filesystem::path{paths.front()} / name).string(), pos);
}
auto delete_all_callback(std::shared_ptr<lgo::divy_hard_t>, std::shared_ptr<lgo::divy_soft_t>,
                         load_state_t* state) -> void
{
    if (state->m_clear_page)
        state->m_clear_page();
}
auto delete_all_confirm_callback(std::shared_ptr<lgo::divy_hard_t>, std::shared_ptr<lgo::divy_soft_t>,
                                 std::shared_ptr<lgo::text_line_flex_std_input_t>, load_state_t*) -> void
{
}
auto delete_all_cancel_callback(std::shared_ptr<lgo::divy_hard_t>) -> void {}

auto folder_button_callback(lgo::engine_t* engine, GUI_persistent_t* persistent, curl_multi_worker_t* multi,
                            load_state_t* load_state, blob_queue_t* blob_queue,
                            std::shared_ptr<lgo::scroll_t> viewer_div, std::string* pos_ptr,
                            std::string pos) -> void
{
    viewer_div->set_scroll_offset({});
    *pos_ptr = pos;
    list(engine, persistent, multi, load_state, blob_queue, viewer_div, pos_ptr);
    return;
}

namespace
{
    auto paste_dialog(lgo::engine_t* engine, GUI_persistent_t* persistent, load_state_t* state,
                      std::string destination) -> void
    {
        const auto clipboard{persistent->m_clipboard};
        persistent->m_dialog_queue.push_back([=]
        {
            if (!clipboard)
            {
                persistent->m_dialog = std::make_unique<lgo::modal_dialog_t>(
                    engine, state->m_progress_column->get_depth_tracker(), "貼上",
                    std::vector<std::string>{"請先剪下或複製一個檔案或資料夾。"},
                    std::vector<std::pair<std::string, std::string>>{});
                persistent->m_dialog_done = [](bool, const auto&) { return true; };
                return;
            }
            persistent->m_dialog = std::make_unique<lgo::modal_dialog_t>(
                engine, state->m_progress_column->get_depth_tracker(),
                clipboard->copy ? "複製到目前資料夾" : "剪下並貼上",
                std::vector<std::string>{"項目：" + clipboard->entity.at("name").get<std::string>(),
                    "目的地：目前顯示的資料夾"},
                std::vector<std::pair<std::string, std::string>>{}, "貼上");
            persistent->m_dialog_done = [=](bool accepted, const auto&)
            {
                if (!accepted) return true;
                if (state->m_session != clipboard->session || clipboard->session->clearing)
                {
                    persistent->m_dialog->error("帳戶已變更，請重新剪下或複製");
                    return false;
                }
                begin_directory_task(state, clipboard->session,
                    clipboard->copy ? "directory-copy" : "directory-move", clipboard->source,
                    clipboard->entity, destination, clipboard->entity.at("name").get<std::string>());
                if (!clipboard->copy && persistent->m_clipboard &&
                    persistent->m_clipboard->session == clipboard->session &&
                    persistent->m_clipboard->entity.at("id") == clipboard->entity.at("id") &&
                    !persistent->m_clipboard->copy)
                    persistent->m_clipboard.reset();
                return true;
            };
        });
    }

    auto delete_dialog(lgo::engine_t* engine, GUI_persistent_t* persistent, load_state_t* state,
                       std::shared_ptr<storage_session_t> session, std::string source,
                       nlohmann::json entry) -> void
    {
        persistent->m_dialog_queue.push_back([=]
        {
            if (state->m_session != session || session->clearing) return;
            persistent->m_dialog = std::make_unique<lgo::modal_dialog_t>(
                engine, state->m_progress_column->get_depth_tracker(), "刪除檔案或資料夾",
                std::vector<std::string>{"項目：" + entry.at("name").get<std::string>(),
                    "將永久刪除此項目及所有子內容，無法復原。"},
                std::vector<std::pair<std::string, std::string>>{}, "刪除");
            persistent->m_dialog_done = [=](bool accepted, const auto&)
            {
                if (accepted)
                {
                    if (state->m_session != session || session->clearing)
                    { persistent->m_dialog->error("帳戶已變更，請取消並重新操作"); return false; }
                    begin_directory_task(state, session, "directory-delete", source, entry, {}, entry.at("name").get<std::string>());
                }
                return true;
            };
        });
    }
}

auto list(lgo::engine_t* engine, GUI_persistent_t* persistent, curl_multi_worker_t* multi,
          load_state_t* load_state, blob_queue_t* blob_queue, std::shared_ptr<lgo::scroll_t> viewer_div,
          std::string* pos) -> void
{
    // Use `=` instead of `{}`: `nlohmann::json{ expr }` invokes the `std::initializer_list` constructor
    // and wraps the returned array into a 1-element outer array `[expr]`.
    const auto generation{++persistent->m_listing_generation};
    for (auto& pending : persistent->m_listings)
        pending.control->cancel();
    auto session{load_state->m_session};
    if (session && session->clearing)
    {
        auto contents{std::static_pointer_cast<lgo::divy_flex_soft_t>(viewer_div->content())};
        contents->clear_state();
        contents->set_size({viewer_div->view_size().x,0});
        viewer_div->set_scroll_offset({});
        persistent->m_directory_status->set_text("清除中");
        return;
    }
    auto control{std::make_shared<request_control_t>()};
    control->retry_limit = load_state->m_settings.network_retries;
    auto future{std::async(std::launch::async,
                           [target = *pos, session, control]() mutable
                           {
                               google_easy_t request;
                               if (session)
                                   request.m_account_context = session->account;
                               request.control_set(control);
                               auto entries = list_folder_content(request, &target);
                               auto path = nlohmann::json::array();
                               std::set<std::string> visited;
                               auto ancestor{target};
                               // Read actual parent metadata in the listing worker. Navigation
                               // history becomes stale after moving a folder or restoring tasks.
                               while (session && ancestor != session->root && ancestor != "null")
                               {
                                   if (!visited.insert(ancestor).second)
                                       throw std::runtime_error{"資料夾的父目錄形成循環"};
                                   const auto data = blob_get_run(request, ancestor, "name");
                                   const auto metadata = nlohmann::json::parse(data.at("name").get<std::string>());
                                   path.push_back({{"id", ancestor}, {"name", metadata.at("name")}});
                                   ancestor = metadata.at("parent").get<std::string>();
                               }
                               if (session && ancestor == session->root)
                                   path.push_back({{"id", session->root}, {"name", "根目錄"}});
                               std::reverse(path.begin(), path.end());
                               return nlohmann::json{{"entries", std::move(entries)}, {"path", std::move(path)}};
                           })};
    persistent->m_directory_status->set_text("目錄更新中");
    const auto source{*pos};
    auto apply = [=](nlohmann::json result)
    {
        const auto offset{viewer_div->scroll_offset()};
        auto contents{std::make_shared<lgo::divy_flex_soft_t>(engine, load_state->m_progress_column->get_depth_tracker(),
            storage_content_depth, lgo::pos_2D{}, lgo::size_1D{viewer_div->view_size().x}, lgo::std_margin,
            lgo::alignment_2D{lgo::alignment_x::left,lgo::alignment_y::top})};
        auto path{std::make_shared<lgo::divx_flex_soft_t>(engine, load_state->m_progress_column->get_depth_tracker(),
            storage_content_depth, lgo::pos_2D{},
            lgo::size_1D{lgo::font_preference_t::default_get()[0]->leading_get()}, lgo::std_margin,
            lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::center})};
        for (const auto& ancestor : result.at("path"))
        {
            if (path->len().x)
            {
                // A new text_line_flex_std_t measures its deferred glyphs as
                // zero width here. Measure a loaded label before sizing its clip.
                auto separator{path->add_object_shared<lgo::text_line_std_t>(
                    storage_content_depth, ">", lgo::std_white, lgo::size_1D{0})};
                separator->set_size(separator->get_text_size());
            }
            const auto id{ancestor.at("id").get<std::string>()};
            auto button{path->add_object_shared<progress_button_t>(
                storage_content_depth, [=, viewer = std::weak_ptr{viewer_div}]
                {
                    if (auto target{viewer.lock()}; target && load_state->m_session == session &&
                        !persistent->m_switching && session && !session->clearing)
                        folder_button_callback(engine, persistent, multi, load_state, blob_queue, target, pos, id);
                }, ancestor.at("name").get<std::string>(), lgo::std_white, lgo::size_1D{0})};
            button->set_size(button->get_text_size());
        }
        for (const auto& entry : result.at("entries"))
        {
            const bool folder{entry.at("type") == "folder"};
            if (!folder && entry.at("type") != "file") throw lgo::internal_error_t{"obj type"};
            const auto id{entry.at("id").get<std::string>()};
            const auto name{entry.at("name").get<std::string>()};
            auto row{contents->add_object_shared<lgo::divy_flex_soft_t>(storage_content_depth,
                lgo::size_1D{viewer_div->view_size().x}, lgo::std_margin,
                lgo::alignment_2D{lgo::alignment_x::left,lgo::alignment_y::top})};
            row->add_object<lgo::button_text_t<lgo::text_line_scroll_std_t>>(
                storage_content_depth, [=, viewer = std::weak_ptr{viewer_div}]
                {
                    if (auto target{viewer.lock()}; folder && target)
                        folder_button_callback(engine, persistent, multi, load_state, blob_queue, target, pos, id);
                },
                (folder ? "資料夾，名稱：" : "檔案，名稱：") + name +
                    (folder ? "" : "，大小：" + std::to_string(entry.at("size").get<std::uint64_t>())),
                lgo::std_white, row->space_x(lgo::std_margin));
            auto actions{row->add_object_shared<lgo::divx_soft_t>(
                storage_content_depth, lgo::div_create_from_member{},
                lgo::size_1D{lgo::font_preference_t::default_get()[0]->leading_get()}, lgo::std_margin,
                row->space_x(lgo::std_margin),
                lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::top})};
            actions->add_object<progress_button_t>(storage_content_depth, [=]
                {
                    if (folder) folder_download_callback(load_state, name, id);
                    else file_download_callback(load_state, name, id);
                }, "下載", lgo::std_white, lgo::size_1D{32});
            auto separator = [&]
            {
                actions->add_object<lgo::std_liney_t>(storage_content_depth,
                    lgo::size_1D{lgo::font_preference_t::default_get()[0]->leading_get()}, lgo::std_white);
            };
            separator();
            actions->add_object<progress_button_t>(storage_content_depth, [=]
                { persistent->m_clipboard = GUI_persistent_t::clipboard_t{session, source, entry, false}; },
                "剪下", lgo::std_white, lgo::size_1D{32});
            separator();
            actions->add_object<progress_button_t>(storage_content_depth, [=]
                { persistent->m_clipboard = GUI_persistent_t::clipboard_t{session, source, entry, true}; },
                "複製", lgo::std_white, lgo::size_1D{32});
            separator();
            actions->add_object<progress_button_t>(storage_content_depth, [=]
                { delete_dialog(engine, persistent, load_state, session, source, entry); },
                "刪除", lgo::std_white, lgo::size_1D{32});
            row->add_object<lgo::std_linex_t>(
                storage_content_depth, row->space_x(lgo::std_margin), lgo::std_white);
        }
        const bool same_path{persistent->m_breadcrumb_folder == source};
        const auto path_offset{persistent->m_breadcrumbs->scroll_offset()};
        persistent->m_breadcrumbs->set_content(std::move(path));
        persistent->m_breadcrumb_folder = source;
        persistent->m_breadcrumbs->set_scroll_offset(same_path ? path_offset : lgo::size_2D{});
        viewer_div->set_content(std::move(contents));
        viewer_div->set_scroll_offset(offset);
    };
    persistent->m_listings.push_back({generation, session, control, std::move(future), std::move(apply)});
    return;
}

auto home(lgo::engine_t* engine, lgo::state_t* state, GUI_persistent_t* persistent,
          std::shared_ptr<lgo::divy_soft_t>* progress_column, load_state_t* load_state,
          curl_multi_worker_t* multi, blob_queue_t* blob_queue, std::string* pos) -> void
{
    state->clear_state();
    engine->cursor_highlight_set(true);
    auto main_div{state->add_object_shared<lgo::divy_hard_t>(
        lgo::depth_range_t{0.2f, 1.0f}, lgo::pos_2D{0, 0}, engine->get_window_size(), lgo::size_1D{0},
        lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::top})};

    auto menu_left{main_div->add_object_ghost_shared<lgo::divx_soft_t>(
        storage_content_depth, lgo::div_create_from_member{}, lgo::std_forward, lgo::std_margin,
        main_div->space_x(lgo::std_margin),
        lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::center})};
    menu_left->add_object<lgo::text_line_std_t>(storage_content_depth, "雲端硬碟", lgo::std_white,
                                                main_div->space_x(lgo::std_margin));

    auto menu_right{main_div->add_object_shared<lgo::divx_soft_t>(
        storage_content_depth, lgo::div_create_from_member{}, lgo::std_forward, lgo::std_margin,
        main_div->space_x(lgo::std_margin),
        lgo::alignment_2D{lgo::alignment_x::right, lgo::alignment_y::center})};
    persistent->m_directory_status = menu_right->add_object_shared<lgo::text_line_std_t>(
        storage_content_depth, "", lgo::std_red_light, lgo::size_1D{96});
    auto header_line = main_div->add_object_shared<lgo::std_linex_t>(
        storage_content_depth, main_div->space_x(lgo::std_margin), lgo::std_white);
    auto work_div{main_div->add_object_shared<lgo::divx_soft_t>(
        storage_content_depth, main_div->space_2D(lgo::std_margin), lgo::std_margin,
        lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::top})};

    auto state_div{work_div->add_object_ghost_shared<lgo::divy_soft_t>(
        storage_content_depth, lgo::size_2D{0x100, work_div->space_y(lgo::std_margin).x},
        lgo::std_margin, lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::top})};

    persistent->m_frame_rate = state_div->add_object_shared<lgo::text_line_std_t>(
        storage_content_depth, "frame_rate: evaluating...", lgo::std_white, lgo::size_1D{0x100});
    // The main application loop pauses during interactive resizing; update FPS before each repaint.
    engine->callback_frame_rate_set(
        [frame_rate = std::weak_ptr{persistent->m_frame_rate}](std::uint64_t rate)
        {
            if (auto label{frame_rate.lock()})
            {
                auto text{"frame_rate: " + std::to_string(rate)};
                if (label->get_text() != text)
                {
                    label->set_text(std::move(text));
                }
            }
        });
    persistent->m_blob_queue_cycle_rate = state_div->add_object_shared<lgo::text_line_std_t>(
        storage_content_depth, "blob_queue_cycle_rate: evaluating...", lgo::std_white,
        lgo::size_1D{0x100});
    persistent->m_blob_queue_count = state_div->add_object_shared<lgo::text_line_std_t>(
        storage_content_depth, "blob_queue_count: evaluating...", lgo::std_white,
        lgo::size_1D{0x100});
    // Diagnostics have their own opaque panel. Fixed-width ghost columns used
    // to overflow into each other and into the transfer progress underneath.
    persistent->m_diagnostics_backdrop = state->add_object_shared<lgo::rect_t>(
        lgo::depth_range_t{.18f, .19f}, lgo::pos_2D{}, lgo::size_2D{},
        lgo::color_t{.015f, .015f, .015f});
    persistent->m_diagnostics_summary = state->add_object_shared<lgo::text_line_scroll_std_t>(
        lgo::depth_range_t{.16f, .17f}, lgo::pos_2D{}, "", lgo::std_white, lgo::size_1D{400});
    persistent->m_load_easy_status.clear();
    for (std::size_t i{}; i < load_easy_count; ++i)
        persistent->m_load_easy_status.push_back(state->add_object_shared<lgo::text_line_scroll_std_t>(
            lgo::depth_range_t{.16f, .17f}, lgo::pos_2D{}, "", lgo::std_white, lgo::size_1D{400}));
    auto change_page = [persistent](int delta)
    {
        const auto count{(load_easy_count + persistent->m_diagnostics_page_size - 1) /
                         persistent->m_diagnostics_page_size};
        persistent->m_diagnostics_page = (persistent->m_diagnostics_page + count + delta) % count;
        persistent->show_diagnostics(persistent->m_diagnostics_visible);
    };
    persistent->m_diagnostics_previous = state->add_object_shared<lgo::button_classic_std_t>(
        lgo::depth_range_t{.16f, .17f}, lgo::pos_2D{}, [=] { change_page(-1); }, "上一頁");
    persistent->m_diagnostics_next = state->add_object_shared<lgo::button_classic_std_t>(
        lgo::depth_range_t{.16f, .17f}, lgo::pos_2D{}, [=] { change_page(1); }, "下一頁");
    persistent->m_diagnostics_page_text = state->add_object_shared<lgo::text_line_std_t>(
        lgo::depth_range_t{.16f, .17f}, lgo::pos_2D{}, "", lgo::std_white, lgo::size_1D{400});
    persistent->layout_diagnostics(work_div->get_pos(), work_div->get_size());

    *progress_column = work_div->add_object_shared<lgo::divy_soft_t>(
        storage_content_depth, lgo::size_2D{0x100, work_div->space_y(lgo::std_margin).x},
        lgo::std_margin, lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::bottom}, true);
    (*progress_column)->smooth_children_set(true);
    load_state->m_progress_column = *progress_column;
    auto column_line = work_div->add_object_shared<lgo::std_liney_t>(
        storage_content_depth, work_div->space_y(lgo::std_margin), lgo::std_white);
    auto browser_div{work_div->add_object_shared<lgo::divy_soft_t>(
        storage_content_depth, work_div->space_2D(lgo::std_margin), lgo::std_margin,
        lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::top})};
    persistent->m_breadcrumbs = browser_div->add_object_shared<lgo::scroll_t>(
        storage_content_depth, lgo::size_2D{browser_div->space_x(lgo::std_margin).x,
            lgo::font_preference_t::default_get()[0]->leading_get() + 8}, lgo::scroll_axes_t::x, lgo::std_margin);
    auto viewer_div{browser_div->add_object_shared<lgo::scroll_t>(
        storage_content_depth, browser_div->space_2D(lgo::std_margin), lgo::scroll_axes_t::y, lgo::std_margin)};
    viewer_div->set_content(std::make_shared<lgo::divy_flex_soft_t>(engine, state->get_depth_tracker(),
        storage_content_depth, lgo::pos_2D{}, lgo::size_1D{viewer_div->view_size().x}, lgo::std_margin,
        lgo::alignment_2D{lgo::alignment_x::left,lgo::alignment_y::top}));

    persistent->m_reset_view = [viewer_div, persistent]
    {
        // A completed future may still be waiting for the GUI to collect it.
        // Invalidate it even if cancellation arrived after its last HTTP request.
        ++persistent->m_listing_generation;
        persistent->m_breadcrumbs->set_content({});
        persistent->m_breadcrumb_folder.clear();
        for (auto& listing : persistent->m_listings)
            listing.control->cancel();
        auto contents{std::static_pointer_cast<lgo::divy_flex_soft_t>(viewer_div->content())};
        contents->clear_state();
        contents->set_size({viewer_div->view_size().x,0});
        viewer_div->set_scroll_offset({});
    };
    persistent->m_refresh = [=] { list(engine, persistent, multi, load_state, blob_queue, viewer_div, pos); };
    load_state->m_on_directory_changed = [=](const directory_task_t& task)
    {
        if (task.m_kind == "directory-delete")
        {
            const auto saved = task.m_journal->data();
            const auto ids = saved.value("delete_ids", std::vector<std::string>{});
            if (std::ranges::find(ids, *pos) != ids.end()) *pos = task.m_source;
        }
        if (*pos == task.m_source || *pos == task.m_destination) persistent->m_refresh();
    };
    load_state->m_on_upload_linked = [engine, persistent, multi, load_state, blob_queue, pos,
                                      viewer = std::weak_ptr{viewer_div}](const std::string& uploaded_pos)
    {
        // Navigation already reloads other folders. Refresh only the visible destination
        // so completing an upload never navigates the user away from their current folder.
        if (*pos == uploaded_pos)
        {
            if (auto viewer_div{viewer.lock()})
            {
                list(engine, persistent, multi, load_state, blob_queue, viewer_div, pos);
            }
        }
    };

    auto center_div{state->add_object_shared<lgo::divy_hard_t>(
        lgo::depth_range_t{0.0f, 1.0f}, lgo::pos_2D{0, 0}, engine->get_window_size(), lgo::size_1D{0},
        lgo::alignment_2D{lgo::alignment_x::center, lgo::alignment_y::center})};

    load_state->m_confirm = [engine, state, persistent](std::string message, std::function<void(bool)> done)
    {
        persistent->m_dialog_queue.push_back(
            [=]
            {
                std::vector<std::string> lines;
                std::istringstream stream{message};
                std::string line;
                while (std::getline(stream, line))
                    lines.push_back(line);
                persistent->m_dialog = std::make_unique<lgo::modal_dialog_t>(
                    engine, state->get_depth_tracker(), "覆寫下載目的地", lines,
                    std::vector<std::pair<std::string, std::string>>{}, "刪除並下載");
                persistent->m_dialog_done = [done](bool accepted, const auto&)
                {
                    done(accepted);
                    return true;
                };
            });
    };
    load_state->m_settings_page = [=]
    {
        persistent->m_dialog_queue.push_back(
            [=]
            {
                const auto settings{load_state->m_settings};
                persistent->m_dialog = std::make_unique<lgo::modal_dialog_t>(
                    engine, state->get_depth_tracker(), "設定",
                    std::vector<std::string>{"設定立即生效；進行中的傳輸繼續使用原帳戶。"},
                    std::vector<std::pair<std::string, std::string>>{
                        {"日誌資料夾", settings.logfile_path},
                        {"字型檔案", settings.font_path},
                        {"帳戶檔案", settings.account_path},
                        {"根目錄檔案", settings.root_path},
                        {"網路失敗重試次數（整數或無限）",
                         settings.network_retries < 0 ? "無限" : std::to_string(settings.network_retries)}});
                persistent->m_dialog_done = [=](bool accepted, const std::vector<std::string>& values)
                {
                    if (!accepted)
                        return true;
                    auto settings{load_state->m_settings};
                    try
                    {
                        for (const auto& value : values)
                            if (value.empty())
                                throw std::runtime_error{"路徑與重試次數不可留空"};
                        settings.logfile_path = values[0];
                        settings.font_path = values[1];
                        settings.account_path = values[2];
                        settings.root_path = values[3];
                        if (values[4] == "無限" || values[4] == "-1")
                            settings.network_retries = -1;
                        else
                        {
                            int count{};
                            const auto [end, error]{std::from_chars(
                                values[4].data(), values[4].data() + values[4].size(), count)};
                            if (error != std::errc{} || end != values[4].data() + values[4].size() ||
                                count < 0)
                                throw std::runtime_error{"重試次數必須是非負整數或無限"};
                            settings.network_retries = count;
                        }
                        if (persistent->m_apply_settings)
                            persistent->m_apply_settings(settings);
                        return true;
                    }
                    catch (const std::exception& error)
                    {
                        persistent->m_dialog->error(error.what());
                        return false;
                    }
                };
            });
    };
    load_state->m_clear_page = [=]
    {
        const auto session{load_state->m_session};
        if (!session)
            return;
        persistent->m_dialog_queue.push_back(
            [=]
            {
                persistent->m_dialog = std::make_unique<lgo::modal_dialog_t>(
                    engine, state->get_depth_tracker(), "清除全部 Drive 檔案",
                    std::vector<std::string>{"帳戶：" + session->address,
                                             "將永久刪除此帳戶的全部 Drive 檔案，並取消此帳戶的傳輸。",
                                             "此操作無法復原。請輸入「清除」以確認。"},
                    std::vector<std::pair<std::string, std::string>>{{"確認文字", ""}}, "清除全部");
                persistent->m_dialog_done = [=](bool accepted, const std::vector<std::string>& values)
                {
                    if (!accepted)
                        return true;
                    if (persistent->m_switching)
                    {
                        persistent->m_dialog->error("設定套用中，請稍候");
                        return false;
                    }
                    if (values.front() != "清除")
                    {
                        persistent->m_dialog->error("請輸入「清除」");
                        return false;
                    }
                    if (session != load_state->m_session)
                    {
                        persistent->m_dialog->error("帳戶已變更，請重新開啟清除對話框");
                        return false;
                    }
                    if (persistent->m_clear_account)
                        persistent->m_clear_account();
                    return true;
                };
            });
    };
    persistent->m_paste_button = menu_right->add_object_shared<lgo::button_classic_std_t>(
        storage_content_depth, [=] { paste_dialog(engine, persistent, load_state, *pos); }, "貼上");
    persistent->m_paste_button->set_enabled(false);
    menu_right->add_object<lgo::button_classic_std_t>(
        storage_content_depth,
        [=]
        {
            if (load_state->m_session)
            {
                load_state->m_session->account->retry();
                load_state->m_session->queue->retry();
            }
            persistent->m_refresh();
        },
        "重新整理");
    menu_right->add_object<lgo::button_classic_std_t>(storage_content_depth, load_state->m_settings_page,
                                                      "設定");
    menu_right->add_object<lgo::button_classic_std_t>(
        storage_content_depth, std::bind(&folder_create_callback, engine, persistent, load_state, pos), "新增資料夾");
    menu_right->add_object<lgo::button_classic_std_t>(
        storage_content_depth, [load_state, blob_queue, pos]
        { folder_upload_callback(load_state, blob_queue, *pos); }, "上傳資料夾");
    menu_right->add_object<lgo::button_classic_std_t>(
        storage_content_depth,
        [load_state, blob_queue, pos] { file_upload_callback(load_state, blob_queue, *pos); }, "上傳檔案");
    menu_right->add_object<lgo::button_classic_std_t>(
        storage_content_depth,
        std::bind(&delete_all_callback, center_div, *progress_column, load_state), "清除");
    // Weak references keep the engine callback from retaining an obsolete UI after clear_state().
    engine->callback_window_resize_set(
        [main = std::weak_ptr{main_div}, left = std::weak_ptr{menu_left}, right = std::weak_ptr{menu_right},
         header = std::weak_ptr{header_line}, work = std::weak_ptr{work_div},
         status = std::weak_ptr{state_div}, progress = std::weak_ptr{*progress_column},
         separator = std::weak_ptr{column_line}, browser = std::weak_ptr{browser_div}, viewer = std::weak_ptr{viewer_div},
         center = std::weak_ptr{center_div}, persistent](lgo::size_2D size)
        {
            auto main_div{main.lock()};
            auto menu_left{left.lock()};
            auto menu_right{right.lock()};
            auto header_line{header.lock()};
            auto work_div{work.lock()};
            auto state_div{status.lock()};
            auto progress_column{progress.lock()};
            auto column_line{separator.lock()};
            auto viewer_div{viewer.lock()};
            auto browser_div{browser.lock()};
            auto center_div{center.lock()};
            if (!main_div || !menu_left || !menu_right || !header_line || !work_div || !state_div ||
                !progress_column || !column_line || !browser_div || !viewer_div || !center_div)
            {
                return;
            }
            main_div->set_size(size);
            menu_left->set_size(lgo::size_2D{main_div->space_x(lgo::std_margin).x, menu_left->get_size().y});
            // Reserve the left edge for directory status on narrow windows;
            // all seven toolbar actions still fit at the 800-pixel minimum.
            if (size.x < 1000) menu_left->hide();
            else menu_left->show();
            menu_right->set_size(
                lgo::size_2D{main_div->space_x(lgo::std_margin).x, menu_right->get_size().y});
            // Keep all seven standard-size buttons visible at the minimum width.
            persistent->m_directory_status->set_size({std::clamp<std::int64_t>(
                menu_right->get_size().x - 7 * (lgo::button_classic_std_impl_t::size.x + lgo::std_margin.x),
                0, 96), persistent->m_directory_status->get_size().y});
            header_line->set_size(
                lgo::size_2D{main_div->space_x(lgo::std_margin).x, header_line->get_size().y});
            // space_2D() reserves a gap for another child. These are replacements
            // of the last child, so reclaim both its size and that extra gap.
            auto work_size{main_div->space_2D(lgo::std_margin)};
            work_size.y += work_div->get_size().y + lgo::std_margin.x;
            work_div->set_size(work_size);
            persistent->layout_diagnostics(work_div->get_pos(), work_div->get_size());
            const auto height{work_div->space_y(lgo::std_margin).x};
            state_div->set_size(lgo::size_2D{state_div->get_size().x, height});
            progress_column->set_size(lgo::size_2D{progress_column->get_size().x, height});
            column_line->set_size(lgo::size_2D{column_line->get_size().x, height});
            auto browser_size{work_div->space_2D(lgo::std_margin)};
            browser_size.x += browser_div->get_size().x + lgo::std_margin.x;
            browser_div->set_size(browser_size);
            persistent->m_breadcrumbs->set_size({browser_div->space_x(lgo::std_margin).x,
                lgo::font_preference_t::default_get()[0]->leading_get() + 8});
            auto viewer_size{browser_div->space_2D(lgo::std_margin)};
            viewer_size.y += viewer_div->get_size().y + lgo::std_margin.x;
            viewer_div->set_size(viewer_size);
            auto contents{std::static_pointer_cast<lgo::divy_flex_soft_t>(viewer_div->content())};
            contents->set_size({viewer_div->view_size().x,contents->get_size().y});
            contents->fit_children_sideways();
            // Each entry is itself a div containing a name, actions and separator.
            contents->visit_children([](lgo::GUI_object_t& child)
                { if (auto row{dynamic_cast<lgo::divy_flex_soft_t*>(&child)}) row->fit_children_sideways(); });

            center_div->set_size(size);
        });
    // Below this size the fixed-width desktop controls and status column would overlap.
    glfwSetWindowSizeLimits(engine->window_get(), 800, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);
    list(engine, persistent, multi, load_state, blob_queue, viewer_div, pos);
    return;
}

auto GUI_persistent_t::layout_diagnostics(lgo::pos_2D pos, lgo::size_2D size) -> void
{
    m_diagnostics_backdrop->set_pos(pos);
    m_diagnostics_backdrop->set_size(size);
    constexpr std::int64_t gap{16};
    const auto columns{std::max<std::int64_t>(1, (size.x - gap) / 360)};
    const auto width{(size.x - gap) / columns - gap};
    const auto row_height{m_load_easy_status.front()->get_size().y + 8};
    const auto rows_available{std::max<std::int64_t>(1, (size.y - 112) / row_height)};
    const auto rows_needed{(static_cast<std::int64_t>(load_easy_count) + columns - 1) / columns};
    const auto rows{std::min(rows_available, rows_needed)};
    m_diagnostics_summary->set_pos({pos.x + gap, pos.y + gap});
    m_diagnostics_summary->set_size({size.x - 2 * gap, m_diagnostics_summary->get_size().y});
    m_diagnostics_page_size = columns * rows;
    m_diagnostics_page = std::min(m_diagnostics_page, (load_easy_count - 1) / m_diagnostics_page_size);
    for (std::size_t i{}; i < m_load_easy_status.size(); ++i)
    {
        auto& label{m_load_easy_status[i]};
        const auto slot{i % m_diagnostics_page_size};
        label->set_pos({pos.x + gap + static_cast<std::int64_t>(slot / rows) * (width + gap),
                        pos.y + 64 + static_cast<std::int64_t>(slot % rows) * row_height});
        label->set_size({width, label->get_size().y});
    }
    const auto footer_y{pos.y + size.y - 40};
    m_diagnostics_previous->set_pos({pos.x + gap, footer_y});
    m_diagnostics_next->set_pos({pos.x + gap + 112, footer_y});
    m_diagnostics_page_text->set_pos({pos.x + gap + 240, footer_y});
    show_diagnostics(m_diagnostics_visible);
}

auto GUI_persistent_t::show_diagnostics(bool show) -> void
{
    m_diagnostics_visible = show;
    const auto pages{(load_easy_count + m_diagnostics_page_size - 1) / m_diagnostics_page_size};
    m_diagnostics_page_text->set_text(std::format("F3 連線狀態  {}/{}", m_diagnostics_page + 1, pages), false);
    m_diagnostics_summary->set_text(m_frame_rate->get_text() + "  |  " +
        m_blob_queue_cycle_rate->get_text() + "  |  " + m_blob_queue_count->get_text(), false);
    for (auto& object : std::vector<std::shared_ptr<lgo::GUI_object_t>>{
             m_diagnostics_backdrop, m_diagnostics_summary, m_diagnostics_page_text,
             m_diagnostics_previous, m_diagnostics_next})
        if (show)
            object->show();
        else
            object->hide();
    for (std::size_t i{}; i < m_load_easy_status.size(); ++i)
        if (show && i / m_diagnostics_page_size == m_diagnostics_page)
            m_load_easy_status[i]->show();
        else
            m_load_easy_status[i]->hide();
}

GUI_persistent_t::~GUI_persistent_t()
{
    for (auto& listing : m_listings)
        listing.control->cancel();
    m_dialog_done = {};
    m_dialog.reset();
}
