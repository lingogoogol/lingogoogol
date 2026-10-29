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
            const auto deadline{std::chrono::steady_clock::now() + (phase=="pool" ? 120s : 20s)};
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
            wait([&]
            {
                if (phase=="pool" && task->m_control->paused) task->m_control->retry();
                return task->m_result != task_result_t::active;
            });
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
        if (phase == "unlink-file" || phase == "unlink-folder" || phase == "unlink-away")
        {
            const bool folder{phase == "unlink-folder"};
            const bool away{phase == "unlink-away"};
            const auto id{folder ? "visible-folder" : "visible-file"};
            const auto selected{folder ? "visible-leaf" : id};
            wait([&] { return persistent.m_directory_rows.contains(id); });
            if (folder)
            {
                pos = "visible-child";
                persistent.m_refresh();
                wait([&] { return persistent.m_breadcrumb_folder == pos &&
                                  persistent.m_directory_rows.contains(selected); });
            }
            if (away)
            {
                // Navigation keeps the old rows until its replacement arrives.
                // A failing new folder must not keep actions for an old deletion.
                pos = "elsewhere";
                persistent.m_refresh();
                wait([&] { return persistent.m_directory_status->get_text() == "目錄更新失敗"; });
                check(persistent.m_breadcrumb_folder == "root", "Navigation test lost its old visible folder.");
            }
            // Keep a same-named sibling and the actual removed row observable.
            lgo::scroll_t* viewer{};
            std::function<void(lgo::GUI_object_t&)> find_viewer = [&](auto& object)
            {
                if (auto scroll{dynamic_cast<lgo::scroll_t*>(&object)}; scroll && scroll->axes()==lgo::scroll_axes_t::y)
                    viewer = scroll;
                object.visit_children(find_viewer);
            };
            gui.get_object_shared<lgo::divy_hard_t>(0)->visit_children(find_viewer);
            check(viewer, "Missing deletion-test viewport.");
            auto contents{std::static_pointer_cast<lgo::divy_flex_soft_t>(viewer->content())};
            std::weak_ptr<lgo::divy_flex_soft_t> old_row{
                contents->get_object_shared<lgo::divy_flex_soft_t>(persistent.m_directory_rows.at(selected))};
            persistent.m_clipboard = GUI_persistent_t::clipboard_t{session, pos, entry(selected,"shared",!folder),true};
            pump();
            check(persistent.m_paste_button->enabled(), "Test clipboard did not enable paste.");
            const auto old_generation{persistent.m_listing_generation};
            // A pre-unlink listing becomes ready exactly when the change is
            // observed. Its generation must be invalidated before applying it.
            std::promise<json> stale;
            bool stale_applied{}, observed{};
            if (!away)
                persistent.m_listings.push_back({old_generation, session, std::make_shared<request_control_t>(),
                    stale.get_future(), [&](json) { stale_applied = true; }});
            auto changed{loads.m_on_directory_changed};
            loads.m_on_directory_changed = [&](const directory_task_t& task)
            {
                if (observed) { changed(task); return; }
                if (!away) stale.set_value(json::object());
                changed(task);
                check(task.m_result == task_result_t::active && task.m_source_unlinked,
                      "Directory changed only after content deletion completed.");
                check(old_row.expired(), "The unlinked row still owns active GUI controls.");
                check(!persistent.m_clipboard, "Unlinked clipboard selection remained usable.");
                if (!away)
                    check(persistent.m_listing_generation > old_generation, "Unlink did not invalidate the old listing.");
                else
                    check(pos == "elsewhere", "Unlink interrupted navigation to an unrelated folder.");
                if (folder)
                    check(pos == "root" && persistent.m_directory_rows.empty(),
                          "Deleting an ancestor did not clear the old view and navigate to its source.");
                else
                    check(!persistent.m_directory_rows.contains(id) && persistent.m_directory_rows.contains("keep-file"),
                          "Unlink removed the wrong same-named row.");
                observed = true;
                storage_atomic_write("unlink-observed", "yes");
            };
            begin_directory_task(&loads, session, "directory-delete", "root", entry(id,"shared",!folder), {}, "shared");
            auto task{loads.m_task_list.back()};
            wait([&] { return observed; });
            wait([&] { return persistent.m_directory_status->get_text() == "目錄更新失敗"; });
            check(!stale_applied && !persistent.m_paste_button->enabled(),
                  "Stale listing or clipboard became usable after unlink.");
            check(task->m_result == task_result_t::active, "Delete did not remain active while requests were blocked.");
            storage_atomic_write("unlink-release", "yes");
            wait([&] { return task->m_control->paused.load(); });
            check(!persistent.m_directory_rows.contains(id), "A failed content DELETE restored the removed row.");
            task->m_control->retry();
            wait([&] { return task->m_result != task_result_t::active; });
            check(task->m_result == task_result_t::completed, "Unlinked deletion did not recover after retry.");
            check(!stale_applied, "A late listing restored stale controls.");
            loads.m_on_directory_changed = std::move(changed);
        }
        else if (phase=="pool")
        {
            auto file=entry("pool-file","pool",true);
            file["size"]=19500;
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
                for (const auto& task : {std::static_pointer_cast<load_task_t>(copy),download,upload})
                    if (task->m_control->paused) task->m_control->retry();
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
                auto dividers = json::array();
                if (commands.empty())
                {
                    std::int64_t actions_x{};
                    std::function<void(lgo::GUI_object_t&)> find_actions = [&](auto& object)
                    {
                        if (auto text{dynamic_cast<lgo::text_line_t*>(&object)}; text && text->get_text()=="操作")
                            actions_x=text->get_pos().x;
                        object.visit_children(find_actions);
                    };
                    find_actions(*main);
                    std::function<void(lgo::GUI_object_t&)> find_dividers = [&](auto& object)
                    {
                        const auto at{object.get_pos()}; const auto size{object.get_size()};
                        if (size.x>0 && size.y>0 && at.x>=viewer->get_pos().x &&
                            at.y>=crumbs->get_pos().y+crumbs->get_size().y &&
                            at.y+size.y<=viewer->get_pos().y+viewer->view_size().y &&
                            ((dynamic_cast<lgo::std_liney_t*>(&object) && at.x<actions_x) ||
                             dynamic_cast<lgo::std_linex_t*>(&object)))
                            dividers.push_back({at.x+size.x/2,at.y+size.y/2});
                        object.visit_children(find_dividers);
                    };
                    find_dividers(*main);
                }
                storage_atomic_write("nav-input.json", json{{"step",++step},{"commands",commands},
                    {"separators",separators},{"dividers",dividers}}.dump());
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
            auto verify_columns = [&]
            {
                lgo::text_line_t *name_header{}, *size_header{}, *actions_header{};
                std::function<void(lgo::GUI_object_t&)> headings = [&](auto& object)
                {
                    if (auto text{dynamic_cast<lgo::text_line_t*>(&object)})
                    {
                        if (text->get_text()=="檔名") name_header=text;
                        if (text->get_text()=="大小") size_header=text;
                        if (text->get_text()=="操作") actions_header=text;
                    }
                    object.visit_children(headings);
                };
                headings(*main);
                check(name_header && size_header && actions_header,"Missing directory column headings.");
                check(name_header->get_text_size().y<lgo::font_preference_t::default_get()[0]->leading_get(),
                      "Column headings did not use the smaller 12px font.");
                check(name_header->get_pos().y>=crumbs->get_pos().y+crumbs->get_size().y &&
                      name_header->get_pos().y+name_header->get_size().y<=viewer->get_pos().y,
                      "Column headings are not between the path and contents.");
                const auto window_width{engine.get_window_size().x};
                if (window_width==800) check(size_header->get_size().x==0,"Size column stayed visible at minimum width.");
                if (window_width>=900) check(size_header->get_size().x>0,"Size column did not return when widened.");
                check(name_header->get_size().x>=120,"Filename column fell below its minimum width.");
                unsigned rows{}, files{};
                auto contents{std::static_pointer_cast<lgo::divy_flex_soft_t>(viewer->content())};
                contents->visit_children([&](auto& row)
                {
                    lgo::text_line_scroll_std_t* name{};
                    lgo::text_line_std_t* size{};
                    std::vector<progress_button_t*> actions;
                    std::function<void(lgo::GUI_object_t&)> inspect = [&](auto& object)
                    {
                        if (auto label{dynamic_cast<lgo::text_line_scroll_std_t*>(&object)}) name=label;
                        else if (auto button{dynamic_cast<progress_button_t*>(&object)}) actions.push_back(button);
                        else if (auto label{dynamic_cast<lgo::text_line_std_t*>(&object)}) size=label;
                        object.visit_children(inspect);
                    };
                    inspect(row);
                    check(name && size && actions.size()==4,"Entry does not have a name, size and four actions.");
                    check(name->get_pos().x==name_header->get_pos().x &&
                          name->get_size().x==name_header->get_size().x &&
                          size->get_pos().x==size_header->get_pos().x &&
                          size->get_size().x==size_header->get_size().x &&
                          actions.front()->get_pos().x==actions_header->get_pos().x,
                          "Header and entry columns are misaligned after resizing.");
                    for (std::size_t i{}; i<actions.size(); ++i)
                        check(actions[i]->get_text()==std::array{"下載","剪下","複製","刪除"}[i] &&
                              actions[i]->get_size().x==32 && actions[i]->get_pos().y==name->get_pos().y &&
                              actions[i]->get_pos().x>=viewer->get_pos().x &&
                              actions[i]->get_pos().x+32<=viewer->get_pos().x+viewer->view_size().x,
                              "An action is reordered, wrapped, shrunk or clipped outside the viewport.");
                    check(actions.back()->get_pos().x+32==viewer->get_pos().x+viewer->view_size().x,
                          "Actions are not anchored to the right edge.");
                    if (name->get_text()=="size-fixture")
                    {
                        ++files;
                        check(size->get_text()=="18446744073709551615","File size is not isolated in the size column.");
                    }
                    else check(name->get_text()=="nav-6" && size->get_text().empty(),
                               "Folder label includes a type prefix or a size.");
                    ++rows;
                });
                check(rows==2 && files==1,"Column checks did not cover both file and folder entries.");
                return std::pair{name_header->get_size().x,size_header->get_size().x};
            };
            const auto original_pos{viewer->get_pos()}, original_crumb_pos{crumbs->get_pos()};
            const auto original_size{viewer->get_size()};
            for (int cycle{}; cycle<2; ++cycle)
            {
                for (const auto size : {lgo::size_2D{800,600}, lgo::size_2D{1200,900},
                                       lgo::size_2D{820,600}, lgo::size_2D{830,600},
                                       lgo::size_2D{900,700}, lgo::size_2D{1536,1024}})
                {
                    // Resize through Win32/GLFW: changing only the X11 wrapper
                    // with xdotool does not reliably deliver WM_SIZE under Wine.
                    glfwSetWindowSize(engine.window_get(),static_cast<int>(size.x),static_cast<int>(size.y));
                    wait([&] { return engine.get_window_size().x==size.x && engine.get_window_size().y==size.y; });
                    check(crumbs->get_pos().x==viewer->get_pos().x && crumbs->get_size().x==viewer->get_size().x,
                          "Breadcrumb and contents widths diverged after resize.");
                    const auto at{viewer->get_pos()};
                    const auto extent{viewer->get_size()};
                    if (!(at.x>=0 && at.y>=crumbs->get_pos().y+crumbs->get_size().y &&
                          extent.x>100 && extent.y>100 && at.x+extent.x<=size.x && at.y+extent.y<=size.y))
                        throw std::runtime_error{std::format(
                            "Resize to {}x{} left viewport at ({},{}) sized {}x{}, breadcrumbs at ({},{}) sized {}x{}.",
                            size.x,size.y,at.x,at.y,extent.x,extent.y,crumbs->get_pos().x,crumbs->get_pos().y,
                            crumbs->get_size().x,crumbs->get_size().y)};
                    verify_columns();
                    input(json::array()); // Check rendered path pixels at every size.
                }
                if (!(viewer->get_pos().x==original_pos.x && viewer->get_pos().y==original_pos.y &&
                      viewer->get_size().x==original_size.x && viewer->get_size().y==original_size.y &&
                      crumbs->get_pos().x==original_crumb_pos.x && crumbs->get_pos().y==original_crumb_pos.y))
                    throw std::runtime_error{std::format(
                        "Repeated resizing changed viewport ({},{}) {}x{} to ({},{}) {}x{}, breadcrumbs ({},{}) to ({},{}).",
                        original_pos.x,original_pos.y,original_size.x,original_size.y,
                        viewer->get_pos().x,viewer->get_pos().y,viewer->get_size().x,viewer->get_size().y,
                        original_crumb_pos.x,original_crumb_pos.y,crumbs->get_pos().x,crumbs->get_pos().y)};
            }
            const auto threshold{engine.get_window_size().x-verify_columns().first+120};
            for (const auto width : {threshold,threshold-1,threshold+1})
            {
                glfwSetWindowSize(engine.window_get(),static_cast<int>(width),600);
                wait([&] { return engine.get_window_size().x==width; });
                const auto [name_width,size_width]{verify_columns()};
                if (width==threshold)
                    check(name_width==120 && size_width>0,"Size disappeared before the name reached its minimum.");
                if (width<threshold)
                    check(size_width==0,"Size stayed visible after the filename reached its minimum.");
                if (width>threshold)
                    check(name_width==121 && size_width>0,"Size did not recover at the column threshold.");
                input(json::array());
            }
            glfwSetWindowSize(engine.window_get(),800,600);
            wait([&] { return engine.get_window_size().x==800; });
            const auto right{viewer->get_pos().x+viewer->view_size().x};
            const auto row_y{viewer->get_pos().y+12};
            input({{"move",right-116,row_y},{"click"}});
            check(persistent.m_clipboard && !persistent.m_clipboard->copy &&
                  persistent.m_clipboard->entity.at("id")=="nav-6",
                  "Cut did not hit the right-hand action on a narrow window.");
            input({{"move",right-66,row_y},{"click"}});
            check(persistent.m_clipboard && persistent.m_clipboard->copy &&
                  persistent.m_clipboard->entity.at("id")=="nav-6",
                  "Copy did not hit the right-hand action on a narrow window.");
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
