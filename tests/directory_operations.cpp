#include <nlohmann/json.hpp>
import std;
import lgo.io;
import lgo.math;
import lgo.GUI;
import storage.http;
import storage.google;
import storage.kernel;
using namespace std::chrono_literals;
using nlohmann::json;

void check(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error{message};
}
int main(int argc, char** argv)
{
    try
    {
        check(argc == 2, "Expected a test phase.");
        const std::string phase{argv[1]};
        lgo::init_logfile("logfile/directory.txt");
        lgo::FreeType_t freetype;
        lgo::font_face_t::freetype_set(&freetype);
        lgo::font_face_t face{"font/NotoSansTC-VariableFont_wght.ttf", 16};
        lgo::font_preference_t::default_get().assign(1, &face);
        lgo::engine_t engine{lgo::size_2D{1536, 1024}, "Directory operation regression"};
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
        lgo::state_t gui{&engine, &depth};
        std::atomic_bool close{};
        storage_runtime_t runtime{&close};
        storage_settings_t settings;
        settings.network_retries = 0;
        auto session{runtime.open(settings, std::make_shared<request_control_t>())};
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
                    task->m_control->cancel();
                runtime.stop();
                multi.stop();
            }
        } shutdown{close, runtime, loads, multi};
        std::string pos{session->root};
        std::shared_ptr<lgo::divy_soft_t> column;
        home(&engine, &gui, &persistent, &column, &loads, &multi, session->queue.get(), &pos);
        auto pump = [&] {
            check(!message_loop(&engine, &persistent, &multi, &loads, session->queue.get()),
                  "Unexpected window close.");
            lgo::text_line_t::pending_load();
            check(persistent.m_easy_idle.size()+loads.m_end.size()==load_easy_count,
                  "Transfers lost or allocated an extra physical pool slot.");
            for (const auto& [key, callback] : loads.m_end)
                check(persistent.m_easy_status.contains(key),"A request used a handle outside the shared pool.");
        };
        auto wait = [&](auto done) {
            const auto deadline{std::chrono::steady_clock::now() + (phase=="pool" ? 60s : 20s)};
            while (!done())
            {
                check(std::chrono::steady_clock::now() < deadline, "Directory operation timed out.");
                pump();
            }
        };
        auto run = [&](std::string kind, std::string source, json entry, std::string destination,
                       std::string name, bool success = true) {
            begin_directory_task(&loads, session, kind, source, entry, destination, name);
            auto task{loads.m_task_list.back()};
            wait([&] { return task->m_result != task_result_t::active; });
            if (task->m_result != (success ? task_result_t::completed : task_result_t::failed))
                throw std::runtime_error{task->m_progress_text->get_text()};
            check(task->m_confirm_button->get_size().x == 32 && task->m_confirm_button->get_size().y < 32,
                  "Progress actions are not compact text buttons.");
            task->m_dismissed = true;
            pump();
            return task;
        };
        auto entry = [](std::string id, std::string name, bool file = false) {
            json result{{"id", id}, {"name", name}, {"type", file ? "file" : "folder"}};
            if (file)
                result["size"] = 0;
            return result;
        };
        if (phase=="pool")
        {
            auto file=entry("pool-file","pool",true);
            file["size"]=384;
            storage_atomic_write("pool-stage","move");
            run("directory-move","root",file,"destination","pool");
            storage_atomic_write("pool-stage","copy");
            begin_directory_task(&loads,session,"directory-copy","destination",file,"root","pool");
            auto copy{std::static_pointer_cast<directory_task_t>(loads.m_task_list.back())};
            wait([&] { return copy->m_total>0 || copy->m_result!=task_result_t::active; });
            check(copy->m_result==task_result_t::active,"Copy failed during discovery.");
            begin_task(&loads,session,"download-file","pool.bin","pool-file");
            auto download{loads.m_task_list.back()};
            begin_task(&loads,session,"upload-file","empty.bin","root");
            auto upload{loads.m_task_list.back()};
            std::size_t peak{};
            wait([&]
            {
                peak=std::max(peak,loads.m_end.size());
                return loads.m_task_list.empty();
            });
            for (const auto& task : {std::static_pointer_cast<load_task_t>(copy),download,upload})
                check(task->m_result==task_result_t::completed,"Mixed copy/upload/download did not complete.");
            check(peak==load_easy_count,"Directory operations did not fill the shared 64-slot pool.");
            check(storage_read("pool.bin")==storage_read("expected.bin"),"Concurrent download changed file contents.");
            check(copy->m_completed==copy->m_total,"Parallel copy progress did not reach its total.");
            storage_atomic_write("pool-stage","delete");
            run("directory-delete","destination",file,{},"pool");
            // The copied graph must still be independently readable after deleting its source.
            storage_atomic_write("pool-stage","verify");
            google_easy_t request;
            auto root=std::string{"root"};
            const auto entries=list_folder_content(request,&root);
            const auto copied=std::ranges::find_if(entries,[&](const auto& item)
                { return item.at("name")=="pool" && item.at("id")!="pool-file"; });
            check(copied!=entries.end(),"Parallel copy was not published.");
            begin_task(&loads,session,"download-file","copy.bin",copied->at("id").template get<std::string>());
            auto check_copy{loads.m_task_list.back()};
            wait([&] { return check_copy->m_result!=task_result_t::active; });
            check(check_copy->m_result==task_result_t::completed && storage_read("copy.bin")==storage_read("expected.bin"),
                  "Deleting the source damaged the independent parallel copy.");
            storage_atomic_write("pool-stage","retry");
            begin_directory_task(&loads,session,"directory-copy","root",entry("tiny-file","tiny",true),"root","tiny");
            auto retry{loads.m_task_list.back()};
            wait([&] { return retry->m_control->paused.load(); });
            check(retry->m_result==task_result_t::active,"Network failure discarded the pending directory request.");
            retry->m_control->retry();
            wait([&] { return retry->m_result!=task_result_t::active; });
            check(retry->m_result==task_result_t::completed,"Small-file copy did not recover after retry.");
            for (int i{};i<5;++i) pump();
            check(persistent.m_easy_idle.size()==load_easy_count,"Completed operations did not return all pool slots.");
            // Once event dispatch stops, a queued coordinator must still be
            // destructible: no GUI callback can fulfil its promise at shutdown.
            begin_directory_task(&loads,session,"directory-copy","root",entry("tiny-file","tiny",true),"root","tiny");
            auto pending{std::static_pointer_cast<directory_task_t>(loads.m_task_list.back())};
            const auto deadline{std::chrono::steady_clock::now()+5s};
            while (true)
            {
                { std::unique_lock lock{pending->m_requests_mutex}; if (!pending->m_requests.empty()) break; }
                check(std::chrono::steady_clock::now()<deadline,"Coordinator did not queue its first request.");
                std::this_thread::sleep_for(1ms);
            }
            auto journal{pending->m_journal};
            const auto stopped{std::chrono::steady_clock::now()};
            loads.m_task_list.clear();
            pending.reset();
            check(std::chrono::steady_clock::now()-stopped<1s,"Queued coordinator blocked destruction without GUI dispatch.");
            journal->finish();
        }
        else if (phase == "breadcrumbs")
        {
            auto main{gui.get_object_shared<lgo::divy_hard_t>(0)};
            lgo::scroll_t* viewer{};
            std::function<void(lgo::GUI_object_t&)> find_viewer = [&](auto& object)
            {
                if (auto scroll{dynamic_cast<lgo::scroll_t*>(&object)}; scroll && scroll->axes()==lgo::scroll_axes_t::y)
                    viewer=scroll;
                object.visit_children(find_viewer);
            };
            find_viewer(*main);
            check(viewer,"Missing directory contents viewport.");
            auto crumbs{persistent.m_breadcrumbs};
            unsigned step{};
            auto input = [&](json commands)
            {
                auto separators = json::array();
                if (commands.empty() && crumbs->content()) crumbs->content()->visit_children([&](auto& child)
                {
                    if (auto text{dynamic_cast<lgo::text_line_t*>(&child)}; text && text->get_text()==">")
                    {
                        const auto visible{lgo::clip_rect_t{crumbs->get_pos(),crumbs->view_size()}.intersect(
                            {text->get_pos(),text->get_size()})};
                        if (visible.size.x>0 && visible.size.x==text->get_size().x &&
                            visible.size.y==text->get_size().y)
                            separators.push_back({visible.pos.x,visible.pos.y,
                                visible.pos.x+visible.size.x,visible.pos.y+visible.size.y});
                    }
                });
                storage_atomic_write("nav-input.json", json{{"step",++step},{"commands",commands},
                    {"separators",separators}}.dump());
                wait([&] { return std::filesystem::exists("nav-ack") && storage_read("nav-ack")==std::to_string(step); });
            };
            auto names = [&]
            {
                std::vector<std::string> result;
                if (crumbs->content()) crumbs->content()->visit_children([&](auto& child)
                {
                    if (auto button{dynamic_cast<progress_button_t*>(&child)}) result.push_back(button->get_text());
                    else if (auto text{dynamic_cast<lgo::text_line_t*>(&child)})
                    {
                        check(text->get_text()==">","Breadcrumb separator is not >.");
                        check(text->get_size().x>0 && text->get_clip_size().x>0,
                              "Breadcrumb separator has zero width and cannot render.");
                    }
                });
                return result;
            };
            auto click_crumb = [&](const std::string& name)
            {
                lgo::GUI_object_t* button{};
                crumbs->content()->visit_children([&](auto& child)
                    { if (auto text{dynamic_cast<progress_button_t*>(&child)}; text && text->get_text()==name) button=text; });
                check(button,"Missing ancestor button.");
                const auto bounds{lgo::clip_rect_t{crumbs->get_pos(),crumbs->view_size()}.intersect({button->get_pos(),button->get_size()})};
                check(bounds.size.x>0 && bounds.size.y>0,"Ancestor button is clipped out.");
                input({{"move",bounds.pos.x+bounds.size.x/2,bounds.pos.y+bounds.size.y/2},{"click"}});
            };
            wait([&] { return persistent.m_breadcrumb_folder=="root"; });
            pos="nav-7"; persistent.m_refresh();
            wait([&] { return persistent.m_breadcrumb_folder=="nav-7"; });
            check(names().size()==9 && names().front()=="根目錄","Direct navigation did not load every actual ancestor.");
            input(json::array()); // Verify rendered separator pixels before moving the path.
            check(viewer->get_pos().y>=crumbs->get_pos().y+crumbs->get_size().y &&
                  crumbs->max_scroll().x>0,"Breadcrumbs are not a separate horizontal viewport above contents.");
            bool old_button{};
            std::function<void(lgo::GUI_object_t&)> inspect = [&](auto& object)
            {
                if (auto text{dynamic_cast<lgo::text_line_t*>(&object)}; text && text->get_text()=="返回根目錄") old_button=true;
                object.visit_children(inspect);
            };
            inspect(*viewer);
            check(!old_button,"Old root-return row is still present.");
            const auto at{crumbs->get_pos()};
            input({{"move",at.x+100,at.y+8},{"wheel",5}});
            check(crumbs->scroll_offset().x==0,"Vertical wheel moved breadcrumbs.");
            input(json::array({{"wheel",7},{"keydown","Shift_L"},{"wheel",5},{"keyup","Shift_L"}}));
            wait([&] { return !crumbs->scrolling(); });
            check(crumbs->scroll_offset().x==64 && crumbs->scroll_offset().y==0,"Horizontal/Shift wheel did not move breadcrumbs.");
            auto thumb{crumbs->scrollbar_pos(true)};
            input({{"move",thumb.x+2,thumb.y},{"down"},{"move",1530,thumb.y},{"up"}});
            check(crumbs->scroll_offset().x==crumbs->max_scroll().x,"Breadcrumb thumb did not drag to the end.");
            const auto parent{names().at(7)};
            click_crumb(parent);
            wait([&] { return persistent.m_breadcrumb_folder=="nav-6"; });
            check(pos=="nav-6" && names().size()==8 && crumbs->scroll_offset().x==0,"Ancestor click did not truncate the path.");
            input(json::array());
            click_crumb("根目錄");
            wait([&] { return persistent.m_breadcrumb_folder=="root"; });
            check(pos=="root" && names().size()==1,"Root breadcrumb did not navigate.");
            google_easy_t request;
            blob_update_name_run(request,"nav-5",json{{"name","moved-parent"},{"parent","root"},{"next","null"}}.dump());
            pos="nav-5"; persistent.m_refresh();
            wait([&] { return persistent.m_breadcrumb_folder=="nav-5"; });
            check(names()==std::vector<std::string>{"根目錄","moved-parent"},"Breadcrumbs used stale navigation history after a move.");
            input(json::array());
            persistent.m_reset_view();
            check(!crumbs->content() && persistent.m_breadcrumb_folder.empty(),"Reset retained an old account path.");
        }
        else if (phase == "backend")
        {
            engine.device_lost();
            wait([&] { return engine.gpu_available(); });
            for (auto name : {"", ".", "..", "a/b", "a\\b", "  "})
            {
                bool failed{};
                try
                {
                    directory_validate_name(name);
                }
                catch (const std::exception&)
                {
                    failed = true;
                }
                check(failed, "Invalid folder name was accepted.");
            }
            run("directory-create", "missing-folder", {}, {}, "stale-source", false);
            run("directory-create", "root", {}, {}, "測試資料夾");
            run("directory-create", "root", {}, {}, "測試資料夾");
            const auto batched{run("directory-copy", "root", entry("batched-0", "batched", true), "root", "batched")};
            const auto allocations = batched->m_journal->data().at("allocations");
            check(allocations.at("copied-blobs:0").size() == 100 &&
                  allocations.at("copied-blobs:100").size() == 1,
                  "Copy did not durably reserve IDs in bounded batches.");
            run("directory-copy", "root", entry("file", "file", true), "root", "file");
            run("directory-copy", "root", entry("tree", "tree"), "child", "tree", false);
            run("directory-copy", "root", entry("tree", "tree"), "destination", "tree");
            google_easy_t copied_request;
            std::string copied_destination{"destination"};
            const auto copied_entries = list_folder_content(copied_request, &copied_destination);
            json copied_tree;
            for (const auto& value : copied_entries)
                if (value.at("name") == "tree")
                    copied_tree = value;
            check(!copied_tree.empty() && copied_tree.at("id") != "tree",
                  "Copy reused the source folder ID.");
            auto tree_id{copied_tree.at("id").get<std::string>()};
            const auto copied_children = list_folder_content(copied_request, &tree_id);
            auto child_id{copied_children.at(0).at("id").get<std::string>()};
            const auto copied_files = list_folder_content(copied_request, &child_id);
            const auto file_id{copied_files.at(0).at("id").get<std::string>()};
            const auto chunks = json::parse(blob_get_content_run(copied_request, file_id));
            const auto chunk_id{chunks.at(0).get<std::string>()};
            const auto data = json::parse(blob_get_content_run(copied_request, chunk_id));
            const auto data_id{data.at(0).get<std::string>()};
            check(child_id != "child" && file_id != "nested" && chunk_id != "chunk" && data_id != "data",
                  "Copy shared a child, file, chunk or data blob with its source.");
            check(blob_get_content_run(copied_request, data_id) ==
                      blob_get_content_run(copied_request, "data"),
                  "Copied data bytes differ from the source.");
            run("directory-delete", "destination", copied_tree, {}, "tree");
            check(blob_get_content_run(copied_request, "data") == "YWJj",
                  "Deleting the copy changed the source.");
            run("directory-move", "root", entry("file", "file", true), "destination", "file");
            run("directory-move", "root", entry("tree", "tree"), "destination", "tree");
            run("directory-move", "destination", entry("tree", "tree"), "child", "tree", false);
            run("directory-move", "destination", entry("tree", "tree"), "tree", "tree", false);
            run("directory-move", "destination", entry("tree", "tree"), "destination", "tree");
            run("directory-move", "root", entry("conflict", "conflict"), "destination", "conflict");
            google_easy_t request;
            std::string destination{"destination"};
            const auto contents = list_folder_content(request, &destination);
            check(std::ranges::count_if(
                      contents, [](const auto& value) { return value.at("name") == "conflict"; }) == 2,
                  "Moving a same-named folder replaced an existing entry.");
            run("directory-delete", "destination", entry("conflict", "conflict"), {}, "conflict");
            run("directory-delete", "root", entry("malformed", "malformed"), {}, "malformed", false);
            run("directory-delete", "destination", entry("tree", "tree"), {}, "tree");
            // Two independent jobs share the real upload publication chain.
            begin_task(&loads, session, "upload-file", "empty.bin", "root");
            begin_directory_task(&loads, session, "directory-create", "root", {}, {}, "parallel");
            wait([&] { return loads.m_task_list.empty(); });
            for (const auto& task : loads.m_finished_tasks)
                check(task->m_result == task_result_t::completed,
                      "Concurrent publication lost an operation.");
        }
        else if (phase == "copy-crash")
        {
            begin_directory_task(&loads, session, "directory-copy", "root", entry("restart", "restart"),
                                 "destination", "restart");
            auto task{std::static_pointer_cast<directory_task_t>(loads.m_task_list.back())};
            wait([&] { return task->m_completed >= 1; });
            std::_Exit(73);
        }
        else if (phase == "move-crash" || phase == "delete-crash")
        {
            const bool move{phase == "move-crash"};
            begin_directory_task(&loads, session, move ? "directory-move" : "directory-delete",
                                 move ? "root" : "destination", entry("restart", "restart"),
                                 move ? "destination" : "", "restart");
            auto task{std::static_pointer_cast<directory_task_t>(loads.m_task_list.back())};
            wait([&] { return task->m_completed >= 1; });
            std::_Exit(73); // No destructors; replay must use only the durable task state.
        }
        else
        {
            restore_tasks(&loads, [&](const json&) { return session; });
            check(loads.m_task_list.size() == 1, "Missing crash checkpoint.");
            auto task{loads.m_task_list.back()};
            wait([&] { return task->m_result != task_result_t::active; });
            check(task->m_result == task_result_t::completed, "Restored operation did not finish.");
        }
        pump();
        std::cout << "PASS: " << phase << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
