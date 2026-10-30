#include <nlohmann/json.hpp>
import std;
import lgo.io;
import lgo.math;
import lgo.GUI;
import external.GLFW;
import storage.http;
import storage.google;
import storage.kernel;
using namespace std::chrono_literals;

auto check(bool value, const char* message) -> void
{
    if (!value)
        throw std::runtime_error{message};
}

// Compare removal against a freshly laid out list, including unequal margins.
// This catches sign errors at bottom/right anchors and missing middle-row gaps.
template<bool Y, bool Hard>
auto check_removal(lgo::engine_t* engine, lgo::depth_tracker_t* depth) -> void
{
    for (bool reverse : {false, true})
        for (bool far : {false, true})
            for (int removed{}; removed < 3; ++removed)
            {
                const lgo::alignment_2D alignment{far && !Y ? lgo::alignment_x::right : lgo::alignment_x::left,
                    far && Y ? lgo::alignment_y::bottom : lgo::alignment_y::top};
                using div_t = lgo::GUI_div_impl_t<Y, Hard>;
                div_t actual{engine, depth, storage_content_depth, {200,200}, {300,300}, {0}, alignment, reverse};
                div_t expected{engine, depth, storage_content_depth, {200,200}, {300,300}, {0}, alignment, reverse};
                std::array<std::uint64_t,3> ids{};
                std::array<std::shared_ptr<lgo::divy_soft_t>,3> boxes{}, reference{};
                for (int i{}; i < 3; ++i)
                {
                    const lgo::size_2D size{30+i*11,25+i*9};
                    ids[i] = actual.template add_object<lgo::divy_soft_t>(storage_content_depth,
                        size, lgo::size_1D{5+i*3}, alignment);
                    boxes[i] = actual.template get_object_shared<lgo::divy_soft_t>(ids[i]);
                    if (i != removed)
                        reference[i] = expected.template add_object_shared<lgo::divy_soft_t>(
                            storage_content_depth, size, lgo::size_1D{5+i*3}, alignment);
                }
                actual.remove_object(ids[removed]);
                if (actual.len().x != expected.len().x)
                    throw std::runtime_error{std::format("Removal length: Y={} hard={} reverse={} far={} removed={} actual={} expected={}",
                        Y, Hard, reverse, far, removed, actual.len().x, expected.len().x)};
                for (int i{}; i < 3; ++i)
                    if (i != removed)
                        check(boxes[i]->get_pos() == reference[i]->get_pos(),
                              "Row removal differs from a fresh layout at the same anchor.");
                auto added{actual.template add_object_shared<lgo::divy_soft_t>(storage_content_depth,
                    lgo::size_2D{27,17}, lgo::size_1D{2}, alignment)};
                auto added_reference{expected.template add_object_shared<lgo::divy_soft_t>(storage_content_depth,
                    lgo::size_2D{27,17}, lgo::size_1D{2}, alignment)};
                check(actual.len().x == expected.len().x && added->get_pos() == added_reference->get_pos(),
                      "Adding a row reused the removed last child's margin.");
                for (int i{}; i < 3; ++i)
                    if (i != removed)
                        check(boxes[i]->get_pos() == reference[i]->get_pos(), "Adding after removal shifted surviving rows incorrectly.");
            }
}

int main(int argc, char** argv)
{
    try
    {
        check(argc == 2, "Expected lifecycle phase.");
        const std::string phase{argv[1]};
        if (phase=="layout")
        {
            std::filesystem::create_directory("layout-probe");
            std::filesystem::current_path("layout-probe");
            check(storage_settings_t{}.root_path=="root", "Default root directory changed.");
            storage_atomic_write("settings.json", R"({"root_path":"custom-roots"})");
            check(storage_settings_t::load().root_path=="custom-roots", "Custom root directory changed.");
            storage_atomic_write("settings.json", R"({"root_path":"root/root"})");
            check(storage_settings_t::load().root_path=="root", "Old default introduced an extra root directory.");
            std::atomic_bool close{};
            for (const auto name : {"root", "blob_queue"})
            {
                storage_atomic_write(name,"preserve this legacy state");
                bool rejected{};
                try { storage_runtime_t runtime{&close}; }
                catch (const std::runtime_error& error)
                { rejected = std::string_view{error.what()}.starts_with("Move the existing "); }
                check(rejected && storage_read(name)=="preserve this legacy state",
                      "Legacy data was imported, overwritten or silently ignored.");
                std::filesystem::remove(name);
            }
            std::filesystem::current_path("..");
            std::filesystem::remove_all("layout-probe");
            std::puts("PASS: new root paths, saved settings/journals and manual legacy relocation.");
            return 0;
        }
        lgo::init_logfile("logfile/lifecycle.txt");
        // Exercise real GUI and scheduler against a mock Drive, never live credentials.
        lgo::FreeType_t freetype;
        lgo::font_face_t::freetype_set(&freetype);
        lgo::font_face_t face{"font/NotoSansTC-VariableFont_wght.ttf", 16};
        lgo::font_preference_t::default_get().assign(1, &face);
        lgo::engine_t engine{lgo::size_2D{1536, 1024}, "Task lifecycle regression"};
        struct cleanup_t
        {
            lgo::engine_t& engine;
            ~cleanup_t()
            {
                engine.flush();
                lgo::text_line_t::free();
            }
        } cleanup{engine};
        lgo::depth_tracker_t depth;
        check_removal<true,false>(&engine, &depth);
        check_removal<true,true>(&engine, &depth);
        check_removal<false,false>(&engine, &depth);
        check_removal<false,true>(&engine, &depth);
        lgo::state_t gui{&engine, &depth};
        std::atomic_bool close{};
        storage_runtime_t runtime{&close};
        storage_settings_t settings;
        settings.network_retries = 0;
        auto control{std::make_shared<request_control_t>()};
        auto session{runtime.open(settings, control)};
        google_account_t::default_shared_set(session->account);
        load_state_t loads;
        loads.m_session = session;
        loads.m_settings = settings;
        curl_multi_worker_t multi;
        GUI_persistent_t persistent;
        struct shutdown_t
        {
            std::atomic_bool& close;
            storage_runtime_t& runtime;
            load_state_t& loads;
            curl_multi_worker_t& multi;
            ~shutdown_t()
            {
                close = true;
                for (auto& task : loads.m_task_list)
                    task->m_control->cancelled = true;
                for (auto& task : loads.m_finished_tasks)
                    task->m_control->cancel();
                runtime.stop();
                multi.stop();
            }
        } shutdown{close, runtime, loads, multi};
        std::string pos{session->root};
        std::shared_ptr<lgo::divy_soft_t> column;
        home(&engine, &gui, &persistent, &column, &loads, &multi, session->queue.get(), &pos);
        auto pump = [&]
        {
            check(!message_loop(&engine, &persistent, &multi, &loads, session->queue.get()),
                  "GUI unexpectedly closed.");
            lgo::text_line_t::pending_load();
        };
        if (phase == "upload-crash")
            begin_task(&loads, session, "upload-file", "source.bin", pos);
        else if (phase == "download-crash")
        {
            google_easy_t request;
            const auto content = list_folder_content(request, &pos);
            check(content.size() == 1, "Upload publication is missing or duplicated.");
            begin_task(&loads, session, "download-file", "output.bin", content[0]["id"]);
        }
        else if (phase == "resume-upload" || phase == "resume-download")
        {
            restore_tasks(&loads,
                          [&](const nlohmann::json& intent)
                          {
                              check(intent["account_email"] == session->address, "Saved account changed.");
                              return session;
                          });
            check(loads.m_task_list.size() == 1, "Expected one persisted task.");
        }
        else if (phase == "signout-resume")
        {
            google_easy_t request;
            const auto content = list_folder_content(request,&pos);
            begin_task(&loads,session,"download-file","signedout.bin",content[0]["id"]);
            auto download = std::dynamic_pointer_cast<download_task_t>(loads.m_task_list.back());
            begin_task(&loads,session,"upload-file","second.bin",pos);
            auto upload = loads.m_task_list.back();
            upload->pause(true);
            const auto deadline = std::chrono::steady_clock::now()+40s;
            while (!multi.pending(download->m_control))
            {
                check(std::chrono::steady_clock::now()<deadline,"No in-flight download before sign-out.");
                pump();
            }
            check(download->m_result==task_result_t::active,"Download completed before sign-out fixture.");
            const auto index = storage_read("task");
            for (auto task : {std::static_pointer_cast<load_task_t>(download),upload})
                finish_task(*task,task_result_t::signed_out,"等待帳號登入");
            multi.interrupt();
            loads.m_session.reset();
            runtime.sign_out(session->storage_id);
            while (!loads.m_finished_tasks.empty())
            {
                check(std::chrono::steady_clock::now()<deadline,"Signed-out requests did not drain.");
                pump();
            }
            check(storage_read("task")==index,"Sign-out removed durable checkpoints.");
            check(std::filesystem::exists(download->m_journal->path()/"state"),"Download checkpoint was deleted.");
            session=runtime.open(settings,control);
            loads.m_session=session;
            auto restore = [&](const auto&) { return session; };
            restore_tasks(&loads,restore);
            restore_tasks(&loads,restore);
            check(loads.m_task_list.size()==2,"Login lost tasks or restored duplicates.");
            check(loads.m_task_list.back()->m_control->user_paused,"Login lost explicit pause state.");
            loads.m_task_list.back()->pause(false);
            while (!loads.m_task_list.empty())
            {
                check(std::chrono::steady_clock::now()<deadline,"Resumed transfer did not complete.");
                pump();
            }
            check(storage_read("signedout.bin")==storage_read("source.bin"),"Signed-out download resumed with wrong bytes.");
            check(nlohmann::json::parse(storage_read("task")).empty(),"Resumed task journals remain.");
            std::println("PASS: sign-out drains live transfers, preserves checkpoints and pause state, and login resumes exact bytes once.");
            return 0;
        }
        else if (phase == "pause-controls")
        {
            google_easy_t request;
            const auto content = list_folder_content(request, &pos);
            begin_task(&loads, session, "upload-file", "source.bin", pos);
            auto upload{loads.m_task_list.back()};
            upload->pause(true);
            begin_task(&loads, session, "download-file", "cancel-paused.bin", content[0]["id"]);
            auto download{loads.m_task_list.back()};
            download->pause(true);
            unsigned step{};
            auto click = [&](const auto& button, auto done)
            {
                pump();
                const auto settle_deadline{std::chrono::steady_clock::now() + 5s};
                while (column->children_moving())
                {
                    check(std::chrono::steady_clock::now() < settle_deadline, "Progress layout did not settle.");
                    pump();
                }
                const auto at{button->get_pos()};
                const auto size{button->get_size()};
                storage_atomic_write("click.json", nlohmann::json{{"step", ++step},
                    {"x", at.x + size.x / 2}, {"y", at.y + size.y / 2}}.dump());
                const auto deadline{std::chrono::steady_clock::now() + 10s};
                while (!done())
                {
                    check(std::chrono::steady_clock::now() < deadline, "Transfer action button did not respond.");
                    pump();
                }
            };
            for (auto task : {upload, download})
            {
                click(task->m_resume_button, [&] { return !task->m_control->user_paused; });
                const auto size{std::filesystem::file_size("source.bin")};
                auto bytes = [&]() -> std::uint64_t
                {
                    if (const auto uploading{std::dynamic_pointer_cast<upload_task_t>(task)})
                        return uploading->m_progress;
                    return std::dynamic_pointer_cast<download_task_t>(task)->m_progress;
                };
                auto wait = [&](auto done, const char* message)
                {
                    const auto deadline{std::chrono::steady_clock::now() + 15s};
                    while (!done())
                    {
                        check(std::chrono::steady_clock::now() < deadline, message);
                        pump();
                    }
                };
                wait([&] { return task->m_progress_text->get_text().contains(
                                      "0B/" + std::to_string(size) + "B"); },
                     "Total size was not shown before the first successful batch.");
                check(bytes() == 0, "Initial total size appeared after data had already completed.");
                wait([&] { return bytes() > 0; }, "Transfer did not reach a nonzero checkpoint.");
                check(bytes() < size, "Transfer completed before pause could be tested.");
                const auto bar{task->m_progress_bar};
                check(bar && bar->fraction() > 0. && bar->fraction() < 1., "Missing nonzero graphical progress.");
                check(bar->get_pos().y >= task->m_progress_text->get_pos().y + task->m_progress_text->get_size().y &&
                      bar->get_pos().y + bar->get_size().y <= task->m_pause_button->get_pos().y,
                      "Progress bar is not between task text and actions.");
                click(task->m_pause_button, [&] { return task->m_control->user_paused.load(); });
                const auto divider{task->m_action_separator};
                check(divider && divider->get_size().x == lgo::std_line_size.x &&
                      divider->get_pos().x >= task->m_resume_button->get_pos().x + task->m_resume_button->get_size().x &&
                      divider->get_pos().x + divider->get_size().x <= task->m_cancel_button->get_pos().x,
                      "Progress actions do not have a standard vertical separator between them.");
                const auto progress{task->m_paused_text};
                const auto fraction{task->m_progress_bar->fraction()};
                check(progress.contains(std::to_string(bytes()) + "B/" + std::to_string(size) + "B"),
                      "Pause lost the completed byte count or total size.");
                check(!progress.contains("準備"), "Pause reverted to the preparation label.");
                wait([&] { return !multi.pending(task->m_control); },
                     "Paused task retained a worker handle or a transfer slot.");
                check(persistent.m_easy_idle.size() == load_easy_count,
                      "Paused tasks did not return all production GUI slots.");
                for (const auto& parked : loads.m_parked)
                    check(!parked.request->nonempty(), "GUI parked a physical curl handle.");
                for (int i{}; i < 5; ++i)
                    pump();
                check(task->m_progress_text->get_text() == "已暫停：" + progress,
                      "The GUI loop replaced paused progress.");
                check(task->m_progress_bar->fraction() == fraction, "Pause changed graphical progress.");
                click(task->m_resume_button, [&] { return !task->m_control->user_paused; });
                check(task->m_progress_text->get_text() == progress, "Continue did not restore progress.");
                click(task->m_pause_button, [&] { return task->m_control->user_paused.load(); });
                check(task->m_journal->data().value("user_paused", false), "Pause click was not persisted.");
                check(task->m_journal->data().value("paused_text", std::string{}) == task->m_paused_text,
                      "Paused progress text was not persisted.");
            }
            for (auto task : {upload, download})
                click(task->m_cancel_button, [&] { return task->m_control->cancelled.load(); });
            pump();
            check(nlohmann::json::parse(storage_read("task")).empty(), "Paused cancellation retained tasks.");
            for (auto task : {upload, download})
            {
                const auto cancel_deadline{std::chrono::steady_clock::now() + 10s};
                while (task->m_result == task_result_t::cancelling)
                {
                    check(std::chrono::steady_clock::now() < cancel_deadline, "Cancellation did not drain.");
                    pump();
                }
                check(task->m_result == task_result_t::cancelled && task->m_confirm_button,
                      "Cancelled transfer has no acknowledgement.");
                const auto label{task == upload ? "上傳：source.bin" : "下載：cancel-paused.bin"};
                check(task->m_progress_text->get_text() == std::string{"已取消："} + label,
                      "Cancelled transfer reused its preparation phase as its name.");
                const auto fraction{task->m_progress_bar->fraction()};
                check(fraction > 0., "Cancellation lost graphical progress.");
                const auto remaining_y{download->m_progress_text->get_pos().y};
                const auto height{column->get_object_shared<lgo::divy_flex_soft_t>(task->m_progress_div)->get_size().y};
                click(task->m_confirm_button, [&] { return task->m_dismissed; });
                pump();
                check(task->m_row_removed, "Acknowledged cancelled row remained visible.");
                if (task == upload)
                {
                    const auto destination{remaining_y + height + lgo::std_margin.x};
                    check(column->smooth_children() && column->children_moving() &&
                          download->m_progress_text->get_pos().y < destination,
                          "Removing a row jumped instead of animating.");
                    auto previous{download->m_progress_text->get_pos().y};
                    const auto deadline{std::chrono::steady_clock::now()+5s};
                    while (column->children_moving())
                    {
                        check(std::chrono::steady_clock::now()<deadline,"Progress animation did not finish.");
                        pump();
                        const auto current{download->m_progress_text->get_pos().y};
                        check(current >= previous && current <= destination,"Progress animation moved upward or overshot.");
                        previous=current;
                    }
                    check(download->m_progress_text->get_pos().y == destination,
                          "Acknowledging the bottom task did not move the remaining task downward.");
                }
            }
            // Hold preparation futures so cleanup remains visible independently
            // of network speed. Cover file/folder uploads and downloads alike.
            for (const auto kind : {"upload-file", "upload-folder", "download-file", "download-folder"})
            {
                const bool upload{std::string_view{kind}.starts_with("upload")};
                const bool folder{std::string_view{kind}.ends_with("folder")};
                const auto path{std::string{"cancelling-"} + kind};
                const auto label{std::string{upload ? "上傳：" : "下載："} + path};
                if (folder) std::filesystem::create_directory(path);
                else storage_atomic_write(path, "keep");
                // An unanswered overwrite confirmation leaves downloads in
                // preparation without deleting the original destination.
                loads.m_confirm = [](std::string, auto) {};
                begin_task(&loads, session, kind, path, upload ? pos : "retry-file");
                auto cancelling{loads.m_task_list.back()};
                check(cancelling->m_progress_text->get_text() == "準備" + label,
                      "A new transfer lost its preparation status.");
                cancelling->pause(true);
                std::promise<void> cleanup_gate;
                if (auto task{std::dynamic_pointer_cast<upload_task_t>(cancelling)})
                {
                    const auto deadline{std::chrono::steady_clock::now() + 10s};
                    while (task->m_prepared.wait_for(0s) != std::future_status::ready)
                    {
                        check(std::chrono::steady_clock::now() < deadline, "Upload preparation stalled.");
                        pump();
                    }
                    task->m_prepared.get();
                    task->m_prepared = cleanup_gate.get_future();
                }
                else std::dynamic_pointer_cast<download_task_t>(cancelling)->m_prepared = cleanup_gate.get_future();
                cancelling->m_control->fail("fixture network failure");
                pump();
                check(cancelling->m_progress_text->get_text() == "網路失敗：" + label + "（fixture network failure）",
                      "Network failure reused the preparation phase as the task name.");
                click(cancelling->m_cancel_button, [&] { return cancelling->m_control->cancelled.load(); });
                for (int i{}; i < 10; ++i) pump();
                check(cancelling->m_result == task_result_t::cancelling && !cancelling->m_row_removed &&
                      cancelling->m_progress_text->get_text() == "取消中：" + label,
                      "Cancellation cleanup lost its status or kept a preparation phase.");
                click(cancelling->m_confirm_button, [&]
                {
                    return std::filesystem::exists("click.ack") && storage_read("click.ack") == std::to_string(step);
                });
                check(!cancelling->m_dismissed, "Cancelling row exposed an acknowledgement button.");
                cleanup_gate.set_value();
                pump();
                check(cancelling->m_result == task_result_t::cancelled &&
                      cancelling->m_progress_text->get_text() == "已取消：" + label,
                      "Finished cleanup did not retain the cancelled status and operation name.");
                for (int i{}; i < 10; ++i) pump();
                check(cancelling->m_progress_text->get_text() == "已取消：" + label,
                      "A later frame replaced the terminal cancellation status.");
                click(cancelling->m_confirm_button, [&] { return cancelling->m_dismissed; });
                pump();
                check(cancelling->m_row_removed, "Cleaned cancellation could not be acknowledged.");
                check(folder ? std::filesystem::is_directory(path) : storage_read(path) == "keep",
                      "Cancelling preparation changed the original source or destination.");
            }
            std::println("PASS: file/folder uploads and downloads keep operation names through failure, cancelling and cancelled; acknowledgement waits for cleanup");
            // Both successful operations retain a full bar, including empty
            // files/folders; use real clicks to acknowledge each terminal row.
            begin_task(&loads, session, "upload-file", "second.bin", pos);
            auto success_upload{loads.m_task_list.back()};
            begin_task(&loads, session, "download-folder", "empty-success", "fast-folder");
            auto success_download{loads.m_task_list.back()};
            const auto complete_deadline{std::chrono::steady_clock::now() + 15s};
            while (success_upload->m_result == task_result_t::active || success_download->m_result == task_result_t::active)
            {
                check(std::chrono::steady_clock::now() < complete_deadline, "Successful transfers did not complete.");
                pump();
            }
            for (auto task : {success_upload, success_download})
            {
                check(task->m_result == task_result_t::completed && !task->m_row_removed,
                      "Successful row was removed automatically.");
                check(task->m_progress_text->get_text().starts_with("已完成：") && task->m_progress_bar->fraction() == 1.,
                      "Successful row lost its Chinese completion status or full bar.");
                click(task->m_confirm_button, [&] { return task->m_dismissed; });
                pump();
                check(task->m_row_removed, "Acknowledged success row remained visible.");
            }
            storage_atomic_write("refused.bin", "keep");
            loads.m_confirm = [](std::string, auto done) { done(false); };
            begin_task(&loads, session, "download-file", "refused.bin", "retry-file");
            auto refused{loads.m_finished_tasks.back()};
            check(refused->m_result == task_result_t::failed, "Refused overwrite was not terminal.");
            click(refused->m_confirm_button, [&] { return refused->m_dismissed; });
            pump();
            check(refused->m_row_removed && storage_read("refused.bin") == "keep", "Refused overwrite acknowledgement failed.");
            check(loads.m_finished_tasks.empty(), "Acknowledged terminal owners were not reclaimed.");
            glfwSetWindowSize(engine.window_get(), 800, 600);
            for (int i{}; i < 10; ++i)
                pump();
            storage_atomic_write("click.json", nlohmann::json{{"step", ++step}, {"key", "keydown"}}.dump());
            const auto deadline{std::chrono::steady_clock::now() + 10s};
            while (!persistent.m_diagnostics_visible)
            {
                check(std::chrono::steady_clock::now() < deadline, "F3 panel did not open.");
                pump();
            }
            const auto previous_page{persistent.m_diagnostics_page};
            click(persistent.m_diagnostics_next,
                  [&] { return persistent.m_diagnostics_page != previous_page; });
            for (int i{}; i < 10; ++i)
                pump(); // Present the updated page number before releasing F3.
            storage_atomic_write("click.json", nlohmann::json{{"step", ++step}, {"key", "keyup"}}.dump());
            while (persistent.m_diagnostics_visible)
            {
                check(std::chrono::steady_clock::now() < deadline, "F3 panel did not close.");
                pump();
            }
            std::println("PASS: real GUI clicks pause, resume, cancel and acknowledge uploads/downloads, success and refusal");
            std::println("PASS: graphical progress stays between text and actions, survives pause/cancel and fills on completion");
            std::println("PASS: initial 0B/size is visible; pause and continue retain nonzero transfer progress");
            std::println("PASS: F3 opens, fits an 800x600 window, pages by mouse and closes on release");
            return 0;
        }
        else if (phase == "pause-save" || phase == "pause-restore")
        {
            if (phase == "pause-save")
            {
                google_easy_t request;
                const auto content = list_folder_content(request, &pos);
                begin_task(&loads, session, "download-file", "paused-output.bin", content[0]["id"]);
                loads.m_task_list.back()->pause(true);
                begin_task(&loads, session, "upload-file", "second.bin", pos);
                loads.m_task_list.back()->pause(true);
            }
            else
                restore_tasks(&loads, [&](const nlohmann::json&) { return session; });
            check(loads.m_task_list.size() == 2, "Paused tasks did not survive restart.");
            for (int i{}; i < 30; ++i)
                pump();
            for (auto& task : loads.m_task_list)
            {
                check(task->m_control->user_paused, "Paused task restarted automatically.");
                check(task->m_journal->data().value("user_paused", false), "Pause was not durable.");
                check(task->m_pause_button->get_pos() == task->m_resume_button->get_pos() &&
                          task->m_pause_button->get_pos() == task->m_retry_button->get_pos(),
                      "Pause/resume/retry buttons moved between states.");
                check(task->m_cancel_button->get_pos().x >=
                          task->m_pause_button->get_pos().x + task->m_pause_button->get_size().x,
                      "Cancel overlaps the primary action.");
            }
            if (phase == "pause-save")
            {
                std::println("PASS: upload/download pause is saved before shutdown; controls share one slot");
                return 0;
            }
            for (auto& task : loads.m_task_list)
            {
                task->m_control->fail("injected error during user pause");
                const auto epoch{task->m_control->epoch.load()};
                task->pause(false);
                check(task->m_control->paused && task->m_control->epoch == epoch &&
                          task->m_control->reason() == "injected error during user pause",
                      "Continue erased a network failure or reset its retry counter.");
                task->m_control->retry();
            }
            const auto deadline{std::chrono::steady_clock::now() + 20s};
            while (!loads.m_task_list.empty())
            {
                check(std::chrono::steady_clock::now() < deadline, "Restored paused task stalled.");
                pump();
            }
            check(storage_read("paused-output.bin") == storage_read("source.bin"), "Paused output differs.");
            std::println("PASS: restored paused tasks continue with exact bytes; Continue preserves network errors");
            return 0;
        }
        else if (phase == "account-switch")
        {
            begin_task(&loads, session, "upload-file", "second.bin", pos);
            auto saved_settings{settings};
            saved_settings.account_path = "second.json";
            auto second{runtime.open(saved_settings, control)};
            check(session->root_path=="root/"+storage_digest(session->storage_id) &&
                  second->root_path=="root/"+storage_digest(second->storage_id) &&
                  session->root_path!=second->root_path && second->root=="second-root",
                  "Service accounts did not use separate hashed roots in the same directory.");
            auto custom_settings = settings;
            custom_settings.root_path = "custom-roots";
            const auto custom_path = "custom-roots/"+storage_digest(session->storage_id);
            storage_atomic_write(custom_path,session->root);
            const auto custom = runtime.open(custom_settings,control);
            check(custom->root_path==custom_path && custom->root==session->root && custom->queue==session->queue,
                  "Custom root directory changed the account or created a separate blob queue.");
            loads.m_session = second;
            google_account_t::default_shared_set(second->account);
            loads.m_settings = saved_settings;
            pos = second->root;
            persistent.m_refresh();
            begin_task(&loads, second, "download-file", "second-output.bin", "retry-file");
            const auto deadline{std::chrono::steady_clock::now() + 20s};
            while (!loads.m_task_list.empty())
            {
                check(std::chrono::steady_clock::now() < deadline, "Account switch stalled.");
                pump();
                for (auto& task : loads.m_task_list)
                    if (task->m_control->paused)
                        task->m_control->retry();
            }
            check(storage_read("second-output.bin") == "ABC", "New account download failed.");
            std::println(
                "PASS: active upload keeps the original account while new requests use the selected account");
            return 0;
        }
        else if (phase == "clear-crash" || phase == "resume-clear")
        {
            session->clearing = true;
            std::atomic_uint64_t progress{}, total{};
            auto clear_control{std::make_shared<request_control_t>()};
            clear_control->retry_limit = 0;
            auto job{std::async(std::launch::async,
                                [&] { return runtime.clear(session, clear_control, progress, total); })};
            const auto deadline{std::chrono::steady_clock::now() + 30s};
            while (job.wait_for(0s) != std::future_status::ready)
            {
                check(std::chrono::steady_clock::now() < deadline, "Clear stalled.");
                pump();
                if (clear_control->paused)
                    clear_control->retry();
                if (phase == "clear-crash" && progress > 0 && progress < total &&
                    std::filesystem::exists("clear-half-applied"))
                    std::_Exit(73);
            }
            auto root{job.get()};
            runtime.cleared(session, root);
            check(root != "root", "Clear did not replace the old root.");
            std::println("PASS: interrupted full-account clear resumes its durable inventory");
            return 0;
        }
        else if (phase == "features")
        {
            const auto original_panel_size{persistent.m_diagnostics_backdrop->get_size()};
            glfwSetWindowSize(engine.window_get(), 800, 600);
            for (int i{}; i < 10; ++i)
                pump();
            check(persistent.m_diagnostics_backdrop->get_size().x < original_panel_size.x,
                  "Window resize did not relayout the F3 panel.");
            glfwSetWindowSize(engine.window_get(), 1536, 1024);
            for (int i{}; i < 10; ++i)
                pump();
            // Every visible diagnostic cell must fit, at both desktop sizes.
            for (auto size : {lgo::size_2D{1536, 940}, lgo::size_2D{768, 516}})
            {
                const lgo::pos_2D origin{16, 64};
                persistent.layout_diagnostics(origin, size);
                for (std::size_t i{}; i < std::min(load_easy_count, persistent.m_diagnostics_page_size); ++i)
                {
                    const auto& label{persistent.m_load_easy_status[i]};
                    const auto a{label->get_pos()};
                    const auto extent{label->get_size()};
                    check(a.x >= origin.x && a.y >= origin.y &&
                              a.x + extent.x <= origin.x + size.x &&
                              a.y + extent.y <= origin.y + size.y - 40,
                          "F3 diagnostic cell extends outside its panel.");
                    for (std::size_t j{}; j < i; ++j)
                    {
                        const auto b{persistent.m_load_easy_status[j]->get_pos()};
                        const auto other{persistent.m_load_easy_status[j]->get_size()};
                        check(a.x >= b.x + other.x || b.x >= a.x + extent.x ||
                                  a.y >= b.y + other.y || b.y >= a.y + extent.y,
                              "F3 diagnostics overlap.");
                    }
                }
            }
            persistent.show_diagnostics(false);
            std::string utf8;
            lgo::text_line_std_input_t::constraint_default(&utf8, U'清');
            lgo::text_line_std_input_t::constraint_default(&utf8, U'除');
            check(utf8 == "清除", "Chinese input is not UTF-8.");
            lgo::text_line_std_input_t::constraint_default(&utf8, U'\b');
            check(utf8 == "清", "Backspace split a UTF-8 codepoint.");
            // Declining an existing destination must leave every byte untouched.
            storage_atomic_write("protected.bin", "keep me");
            loads.m_confirm = [](std::string, auto done) { done(false); };
            begin_task(&loads, session, "download-file", "protected.bin", "test-file");
            auto declined{std::dynamic_pointer_cast<download_task_t>(loads.m_finished_tasks.back())};
            check(!declined->m_error.empty(), "Declined overwrite has no failure reason.");
            check(storage_read("protected.bin") == "keep me",
                  "Declined overwrite destroyed the destination.");
            declined->m_control->cancelled = true;
            pump();
            loads.m_task_list.clear();
            // A transport failure pauses one task, then an explicit retry resumes it.
            begin_task(&loads, session, "download-file", "retry.bin", "retry-file");
            auto retry{loads.m_task_list.back()};
            const auto deadline{std::chrono::steady_clock::now() + 10s};
            while (!retry->m_control->paused)
            {
                check(std::chrono::steady_clock::now() < deadline, "Failure did not pause.");
                pump();
            }
            check(retry->m_retry_button != nullptr && retry->m_cancel_button != nullptr,
                  "Retry/cancel controls are missing.");
            retry->m_control->retry();
            while (!loads.m_task_list.empty())
            {
                check(std::chrono::steady_clock::now() < deadline, "Retry stalled.");
                pump();
            }
            check(storage_read("retry.bin") == "ABC", "Retry output differs.");
            // Cancellation must drain handles without writing late responses.
            begin_task(&loads, session, "download-file", "cancel.bin", "cancel-file");
            auto cancelled{loads.m_task_list.back()};
            cancelled->m_control->cancelled = true;
            for (int i{}; i < 30; ++i)
                pump();
            check(nlohmann::json::parse(storage_read("task")).empty(), "Cancelled task remains resumable.");
            // Browsing remains interactive and discards a stale result.
            pos = "slow-folder";
            persistent.m_refresh();
            check(persistent.m_directory_status->get_text() == "目錄更新中", "Missing directory status.");
            pos = "fast-folder";
            persistent.m_refresh();
            const auto listing_deadline{std::chrono::steady_clock::now()+10s};
            while (!persistent.m_listings.empty())
            {
                check(std::chrono::steady_clock::now()<listing_deadline,"Browsing did not finish.");
                pump();
            }
            check(persistent.m_breadcrumb_folder=="fast-folder" && persistent.m_directory_status->get_text().empty(),
                  "Stale browsing replaced the current ancestor path or left an error.");
            // Render reusable modal objects and exercise focus cleanup across close.
            loads.m_settings_page();
            pump();
            check(persistent.m_dialog != nullptr, "Settings dialog missing.");
            persistent.m_dialog_done = {};
            persistent.m_dialog.reset();
            loads.m_clear_page();
            pump();
            check(persistent.m_dialog != nullptr, "Clear dialog missing.");
            persistent.m_dialog_done = {};
            persistent.m_dialog.reset();
            for (auto& listing : persistent.m_listings)
                listing.control->cancelled = true;
            loads.m_task_list.clear();
            std::println(
                "PASS: overwrite refusal, UTF-8, retry, cancellation, asynchronous browsing and GUI dialogs");
            return 0;
        }
        else
            throw std::runtime_error{"Unknown phase."};
        auto owner{loads.m_task_list.front()};
        const auto deadline{std::chrono::steady_clock::now() + 40s};
        while (!loads.m_task_list.empty())
        {
            check(std::chrono::steady_clock::now() < deadline, "Lifecycle transfer stalled.");
            pump();
            std::uint64_t bytes{}, total{};
            if (auto upload = std::dynamic_pointer_cast<upload_task_t>(owner))
            {
                if (!upload->m_prepared.valid())
                {
                    bytes = upload->m_progress;
                    total = upload->m_size;
                }
            }
            if (auto download = std::dynamic_pointer_cast<download_task_t>(owner))
            {
                if (!download->m_prepared.valid())
                {
                    bytes = download->m_progress;
                    total = download->m_size;
                }
            }
            if (phase.ends_with("crash") && bytes && bytes < total)
            {
                check(std::filesystem::exists("task"), "Task checkpoint is missing.");
                std::_Exit(73); // Simulate abrupt termination; no destructors or catch blocks.
            }
        }
        check(nlohmann::json::parse(storage_read("task")).empty(), "Completed task remains in the index.");
        if (phase == "resume-download")
            check(storage_read("output.bin") == storage_read("source.bin"),
                  "Restored download bytes differ.");
        std::println("PASS: {}", phase);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::println("FAIL: {}", error.what());
        return 1;
    }
}
