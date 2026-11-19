#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <nlohmann/json.hpp>
import std;
import external.GLFW;
import lgo.io;
import lgo.math;
import lgo.GUI;
import storage.http;
import storage.kernel;
using nlohmann::json;
using namespace std::chrono_literals;
void check(bool ok,const char* text) { if(!ok)throw std::runtime_error{text}; }

int main(int argc,char** argv)
{
    try
    {
        lgo::init_logfile("logfile/render-settings.txt");
        const auto defaults=storage_settings_t::load();
        check(defaults.frame_rate_limit==60 && defaults.reserve_in_flight_limit==64,"Incorrect defaults.");
        for(const auto field:{"frame_rate_limit","reserve_in_flight_limit"})
        {
            for(const auto& invalid:std::vector<json>{0,-1,1.5,"64",nullptr,true})
            {
                storage_atomic_write("settings.json",json{{field,invalid}}.dump());
                bool rejected{};
                try {storage_settings_t::load();}catch(const std::exception&){rejected=true;}
                check(rejected,"Invalid persisted limit was accepted.");
            }
        }
        std::filesystem::remove("settings.json");
        lgo::FreeType_t ft;lgo::font_face_t::freetype_set(&ft);
        lgo::font_face_t face{"font/NotoSansTC-VariableFont_wght.ttf",16};
        lgo::font_preference_t::default_get().assign(1,&face);
        lgo::engine_t engine{{1100,1100},"Render settings regression"};
        struct cleanup_t{lgo::engine_t& e;~cleanup_t(){e.flush();lgo::text_line_t::free();}} cleanup{engine};
        const auto window=glfwGetWin32Window(engine.window_get());
        std::size_t frames{};
        engine.callback_frame_add([&](double){++frames;});
        auto pump_for=[&](auto duration)
        {
            const auto deadline=std::chrono::steady_clock::now()+duration;
            while(std::chrono::steady_clock::now()<deadline)
                check(!engine.message_loop(),"Window closed early.");
        };
        if(argc==2 && std::string_view{argv[1]}=="render")
        {
            SetWindowPos(window,HWND_TOP,20,20,900,900,SWP_SHOWWINDOW);
            engine.frame_rate_limit_set(10);
            frames=0;pump_for(1100ms);
            check(frames>=3 && frames<=12,"10 FPS limit was not enforced.");
            std::println("PASS: 10 FPS ({} frames in 1.1 s)",frames);
            engine.frame_rate_limit_set(20);
            frames=0;pump_for(1100ms);
            check(frames>=5 && frames<=23,"Live 20 FPS limit was not enforced.");
            std::println("PASS: live 20 FPS ({} frames in 1.1 s)",frames);
            const auto stopped=[&](const char* message)
            {
                // Drain the native show/move messages before measuring steady state.
                pump_for(150ms);const auto before=frames;pump_for(350ms);
                check(frames==before,message);
            };
            const auto resumed=[&]
            {
                const auto before=frames;pump_for(400ms);
                check(frames>before,"Rendering did not resume when visible.");
            };
            ShowWindow(window,SW_HIDE);stopped("Hidden window kept rendering.");
            ShowWindow(window,SW_SHOW);resumed();
            ShowWindow(window,SW_MINIMIZE);stopped("Minimized window kept rendering.");
            ShowWindow(window,SW_RESTORE);resumed();
            SetWindowPos(window,nullptr,30000,30000,0,0,SWP_NOSIZE|SWP_NOZORDER);
            stopped("Off-screen window kept rendering.");
            SetWindowPos(window,HWND_TOP,20,20,900,900,SWP_SHOWWINDOW);resumed();
            struct cover_t
            {
                HWND window{CreateWindowExW(0,L"STATIC",L"Opaque test cover",WS_POPUP|WS_VISIBLE,
                    0,0,1500,1150,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr)};
                ~cover_t(){if(window)DestroyWindow(window);}
            } cover;
            check(cover.window!=nullptr,"Cannot create occluding window.");
            SetWindowPos(cover.window,HWND_TOP,0,0,1500,1150,SWP_SHOWWINDOW);
            stopped("Fully covered window kept rendering.");
            SetWindowPos(cover.window,HWND_TOP,0,0,300,300,SWP_SHOWWINDOW);resumed();
            SetWindowLongPtrW(cover.window,GWL_EXSTYLE,WS_EX_LAYERED);
            SetLayeredWindowAttributes(cover.window,0,255,LWA_ALPHA);
            SetWindowPos(cover.window,HWND_TOP,0,0,1500,1150,SWP_SHOWWINDOW);
            stopped("Opaque layered window did not stop rendering.");
            // Transparent overlays must not be treated as opaque occluders.
            SetLayeredWindowAttributes(cover.window,0,100,LWA_ALPHA);
            SetWindowPos(cover.window,HWND_TOP,0,0,1500,1150,SWP_SHOWWINDOW);resumed();
            ShowWindow(cover.window,SW_HIDE);resumed();
            PostMessageW(window,WM_CLOSE,0,0);
            bool closed{};
            for(int i=0;i<10 && !closed;++i)closed=engine.message_loop();
            check(closed,"Window did not close normally.");
            std::println("PASS: hidden/minimized/off-screen/occluded stop, partial/transparent visibility and restore");
        }
        else
        {
            lgo::depth_tracker_t depth;lgo::state_t gui{&engine,&depth};
            load_state_t loads;curl_multi_worker_t multi;GUI_persistent_t persistent;
            std::string pos;std::shared_ptr<lgo::divy_soft_t> column;
            home(&engine,&gui,&persistent,&column,&loads,&multi,nullptr,&pos);
            std::atomic_bool close{};storage_runtime_t runtime{&close};
            persistent.m_apply_settings=[&](auto settings)
            {
                settings.save();loads.m_settings=settings;
                engine.frame_rate_limit_set(settings.frame_rate_limit);
                runtime.reserve_in_flight_limit_set(settings.reserve_in_flight_limit);
            };
            auto snapshot_at=std::chrono::steady_clock::now();
            const auto deadline=snapshot_at+90s;
            while(!std::filesystem::exists("close-test"))
            {
                check(std::chrono::steady_clock::now()<deadline,"GUI test timed out.");
                if(std::filesystem::exists("open-settings"))
                {std::filesystem::remove("open-settings");loads.m_settings_page();}
                check(!message_loop(&engine,&persistent,&multi,&loads,nullptr),"GUI closed early.");
                lgo::text_line_t::pending_load();
                if(std::chrono::steady_clock::now()<snapshot_at)continue;
                snapshot_at=std::chrono::steady_clock::now()+50ms;
                storage_atomic_write("ui.json",json{{"dialog",bool(persistent.m_dialog)},
                    {"values",persistent.m_dialog ? persistent.m_dialog->values():std::vector<std::string>{}},
                    {"fps",loads.m_settings.frame_rate_limit},{"reserve",loads.m_settings.reserve_in_flight_limit},
                    {"width",engine.get_window_size().x},{"height",engine.get_window_size().y}}.dump());
            }
            multi.stop();
            const auto saved=storage_settings_t::load();
            check(saved.frame_rate_limit==loads.m_settings.frame_rate_limit &&
                  saved.reserve_in_flight_limit==loads.m_settings.reserve_in_flight_limit,
                  "Persisted limits did not reload through the production parser.");
            std::println("PASS: settings GUI");
        }
        lgo::log_file("finished_all!\n");
    }
    catch(const std::exception& error){std::println("FAIL: {}",error.what());return 1;}
}
