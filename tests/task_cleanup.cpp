#include <nlohmann/json.hpp>
import std;
import storage.state;
import lgo.io;
import lgo.math;
import lgo.GUI;
import storage.http;
import storage.google;
import storage.kernel;
using namespace std::chrono_literals;

void check(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error{message};
}

int main()
{
    try
    {
        lgo::init_logfile("logfile/cleanup.txt");
        lgo::FreeType_t ft;
        lgo::font_face_t::freetype_set(&ft);
        lgo::font_face_t face{"font/NotoSansTC-VariableFont_wght.ttf",16};
        lgo::font_preference_t::default_get().assign(1,&face);
        lgo::engine_t engine{{1536,1024},"Download completion cleanup regression"};
        struct cleanup_t {
            lgo::engine_t& engine;
            ~cleanup_t() { engine.flush(); lgo::text_line_t::free(); }
        } cleanup{engine};
        lgo::depth_tracker_t depth;
        lgo::state_t gui{&engine,&depth};
        load_state_t loads;
        curl_multi_worker_t multi;
        GUI_persistent_t persistent;
        std::string pos{"null"};
        std::shared_ptr<lgo::divy_soft_t> column;
        home(&engine,&gui,&persistent,&column,&loads,&multi,nullptr,&pos);
        std::size_t rendered{};
        const auto on_frame=engine.callback_frame_add([&](double) { ++rendered; });
        struct frame_guard_t {
            lgo::engine_t& engine;
            lgo::engine_t::callback_frame_t* callback;
            ~frame_guard_t() { engine.callback_frame_remove(callback); }
        } frame_guard{engine,on_frame};
        auto pump=[&] {
            check(!message_loop(&engine,&persistent,&multi,&loads,nullptr),"Window closed.");
            lgo::text_line_t::pending_load();
        };
        pump();
        const auto active=task_journal_t::create({{"name","still active"}});
        for (const auto& id : {"1-1","1-2"})
        {
            const bool upload=std::string_view{id}=="1-2";
            std::shared_ptr<load_task_t> task=upload ?
                std::static_pointer_cast<load_task_t>(std::make_shared<upload_task_t>()) :
                std::static_pointer_cast<load_task_t>(std::make_shared<download_task_t>());
            task->m_state=&loads;
            task->m_progress_div=column->add_object<lgo::divy_flex_soft_t>(
                storage_content_depth,column->space_x(lgo::std_margin),lgo::std_margin,
                lgo::alignment_2D{lgo::alignment_x::left,lgo::alignment_y::top});
            const auto row=column->get_object_shared<lgo::divy_flex_soft_t>(task->m_progress_div);
            task->m_progress_text=row->add_object_shared<lgo::text_line_scroll_std_t>(
                storage_content_depth,"Finishing transfer",lgo::std_green_light,lgo::size_1D{256},false);
            task->m_progress_bar=row->add_object_shared<lgo::progress_bar_t>(
                storage_content_depth,lgo::size_2D{256,10},lgo::std_green);
            task->m_journal=std::make_shared<task_journal_t>(
                std::filesystem::path{"task.data"}/id,nlohmann::json::object());
            loads.m_task_list.push_back(task);
            task->m_self_i=std::prev(loads.m_task_list.end());
            const auto start=std::chrono::steady_clock::now();
            finish_task(*task,task_result_t::completed,upload ? "Upload finished" : "Download finished");
            const auto elapsed=std::chrono::steady_clock::now()-start;
            std::cout << (upload ? "Upload" : "Download") << " completion milliseconds: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << std::endl;
            check(elapsed<250ms,"Completed transfer blocked the GUI deleting checkpoints.");
            check(task->m_result==task_result_t::completed,"Completion was not published.");
            check(task_journal_t::restore().size()==(upload ? 1 : 2),
                  "Completed task was left in the durable index.");
            // A late response must not recreate a directory while cleanup owns it.
            task->m_journal->response("late","ignored");
            task->m_journal->progress("late",{{"ignored",true}});
            check(!task->m_journal->response("late") && !task->m_journal->progress("late"),
                  "Finished journal still exposed cache files during deletion.");
            task->m_journal->finish();
            task->m_dismissed=true;
            task.reset();
            pump();
            check(loads.m_finished_tasks.empty(),"Dismissing the row retained its task.");
        }
        const auto deadline=std::chrono::steady_clock::now()+180s;
        std::size_t frames{};
        while (std::filesystem::exists("task.data/1-1") || std::filesystem::exists("task.data/1-2") || frames<10)
        {
            check(std::chrono::steady_clock::now()<deadline,"Checkpoint cleanup did not finish.");
            pump();
            ++frames;
        }
        check(std::filesystem::exists(active->path()/"state"),"Cleanup removed an active task.");
        check(!std::filesystem::exists("task.data/1-1/late.response"),"Late response recreated finished state.");
        active->finish();
        check(rendered>=10,"GUI did not render during checkpoint cleanup.");
        std::cout << "PASS: GUI loops/rendered frames during cleanup: " << frames << '/' << rendered << '\n';
        lgo::log_file("finished_all!\n");
        return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
