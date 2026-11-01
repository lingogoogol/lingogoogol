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
void check(bool ok,const char* message) { if (!ok) throw std::runtime_error{message}; }
int main(int argc,char** argv)
{
    try
    {
        check(argc==2,"Missing phase.");
        const std::string phase=argv[1];
        lgo::init_logfile("logfile/progress.txt");
        lgo::FreeType_t ft; lgo::font_face_t::freetype_set(&ft);
        lgo::font_face_t face{"font/NotoSansTC-VariableFont_wght.ttf",16};
        lgo::font_preference_t::default_get().assign(1,&face);
        lgo::engine_t engine{{1536,1024},"Progress resume regression"};
        struct cleanup_t { lgo::engine_t& e; ~cleanup_t() { e.flush(); lgo::text_line_t::free(); } } cleanup{engine};
        lgo::depth_tracker_t depth; lgo::state_t gui{&engine,&depth};
        std::atomic_bool close{}; storage_runtime_t runtime{&close};
        const auto settings=storage_settings_t::load();
        auto session=runtime.open_space(settings,settings.spaces.at(0),std::make_shared<request_control_t>());
        load_state_t loads; loads.m_session=session; loads.m_settings=settings;
        curl_multi_worker_t multi; GUI_persistent_t persistent;
        struct shutdown_t {
            std::atomic_bool& c; storage_runtime_t& r; load_state_t& l; curl_multi_worker_t& m;
            ~shutdown_t() { c=true; for (auto& t:l.m_task_list) t->m_control->cancel(); r.stop(); m.stop(); }
        } shutdown{close,runtime,loads,multi};
        std::string pos=session->root; std::shared_ptr<lgo::divy_soft_t> column;
        home(&engine,&gui,&persistent,&column,&loads,&multi,session->queue.get(),&pos);
        const auto deadline=std::chrono::steady_clock::now()+90s;
        auto pump=[&] {
            check(std::chrono::steady_clock::now()<deadline,"Resume timed out.");
            check(!message_loop(&engine,&persistent,&multi,&loads,session->queue.get()),"Window closed.");
            lgo::text_line_t::pending_load();
            for (const auto& t:loads.m_task_list) if (t->m_control->paused) throw std::runtime_error{t->m_control->reason()};
        };
        if (phase=="upload-crash") begin_task(&loads,session,"upload-folder","input/tree",pos);
        else if (phase=="download-crash")
            begin_task(&loads,session,"download-folder","output",nlohmann::json::parse(storage_read("fixture-tree")).get<std::string>());
        else restore_tasks(&loads,[&](const auto&) { return session; });
        check(!loads.m_task_list.empty(),"No task restored.");
        while (!loads.m_task_list.empty())
        {
            pump();
            if (loads.m_task_list.empty()) break;
            if (phase=="upload-crash")
            {
                const auto t=std::dynamic_pointer_cast<upload_task_t>(loads.m_task_list.front());
                if (t->m_progress>1875000) std::_Exit(0);
            }
            if (phase=="download-crash")
            {
                const auto t=std::dynamic_pointer_cast<download_task_t>(loads.m_task_list.front());
                for (const auto& f:t->m_file_arr)
                    if (f.m_written && f.m_written<f.m_size && f.m_chunk_arr.size()>2 && f.m_chunk_arr[2].m_received)
                        std::_Exit(0);
            }
        }
        for (const auto& t:loads.m_finished_tasks) check(t->m_result==task_result_t::completed,"Task failed.");
        check(!phase.ends_with("crash"),"Task completed before interruption checkpoint.");
        if (phase=="upload-resume")
        {
            google_easy_t request;
            const auto entries=list_folder_content(request,&pos,session.get());
            const auto tree=std::ranges::find_if(entries,[](const auto& e) { return e.at("name")=="tree"; });
            check(tree!=entries.end(),"Uploaded tree missing.");
            storage_atomic_write("fixture-tree",tree->at("id").dump());
        }
        else
        {
            check(storage_read("input/tree/payload.bin")==storage_read("output/payload.bin"),"Resumed download differs.");
            check(std::filesystem::is_directory("output/empty-folder"),"Resumed download lost an empty folder.");
        }
        std::cout << "PASS: " << phase << '\n';
        return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
