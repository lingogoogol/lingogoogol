#include <nlohmann/json.hpp>
import std;
import lgo.io;
import lgo.math;
import lgo.GUI;
import storage.http;
import storage.google;
import storage.kernel;
using nlohmann::json;
using namespace std::chrono_literals;
void check(bool ok,const char* text) { if (!ok) throw std::runtime_error{text}; }
int main(int argc,char** argv)
{
    try
    {
        lgo::init_logfile("logfile/batch-mode.txt");
        lgo::FreeType_t ft; lgo::font_face_t::freetype_set(&ft);
        lgo::font_face_t face{"font/NotoSansTC-VariableFont_wght.ttf",16};
        lgo::font_preference_t::default_get().assign(1,&face);
        lgo::engine_t engine{{1536,1024},"Batch mode regression"};
        struct cleanup_t { lgo::engine_t& e; ~cleanup_t() { e.flush(); lgo::text_line_t::free(); } } cleanup{engine};
        lgo::depth_tracker_t depth; lgo::state_t gui{&engine,&depth};
        check(storage_settings_t::load().worker_count==64,"Default worker count changed.");
        for (const auto& invalid : std::vector<json>{0,-1,1.5,"64",nullptr,true})
        {
            storage_atomic_write("settings.json",json{{"worker_count",invalid}}.dump());
            bool rejected{};
            try { storage_settings_t::load(); } catch (const std::exception&) { rejected=true; }
            check(rejected,"Invalid persisted worker count was accepted.");
        }
        std::filesystem::remove("settings.json");
        load_state_t loads; curl_multi_worker_t multi; GUI_persistent_t persistent;
        struct shutdown_t { curl_multi_worker_t& m; ~shutdown_t() { m.stop(); } } shutdown{multi};
        std::string pos; std::shared_ptr<lgo::divy_soft_t> column;
        home(&engine,&gui,&persistent,&column,&loads,&multi,nullptr,&pos);
        auto deadline=std::chrono::steady_clock::now()+100s;
        auto pump=[&]
        {
            check(std::chrono::steady_clock::now()<deadline,"Batch mode test timed out.");
            check(!message_loop(&engine,&persistent,&multi,&loads,nullptr),"Window closed early.");
            lgo::text_line_t::pending_load();
            check(persistent.m_easy_idle.size()+loads.m_end.size()==persistent.m_easy_status.size(),"Transport pool lost a handle.");
        };
        auto wait=[&](auto done) { while (!done()) pump(); };
        auto api=[&](std::string path)
        {
            google_easy_t request; request.path_set(path); request.method_get(); request.run();
            return json::parse(request.payload());
        };
        pump();
        check(persistent.m_easy_status.size()==64 && loads.m_settings.batch_request,"Default mode is not 64 batch slots.");
        if (argc==2 && std::string_view{argv[1]}=="ui")
        {
            persistent.m_apply_settings=[&](auto settings) { settings.save(); loads.m_settings=settings; };
            auto snapshot_at=std::chrono::steady_clock::now();
            while (!std::filesystem::exists("close-test"))
            {
                if (std::filesystem::exists("open-settings"))
                { std::filesystem::remove("open-settings"); loads.m_settings_page(); }
                pump();
                if (std::chrono::steady_clock::now()<snapshot_at) continue;
                snapshot_at=std::chrono::steady_clock::now()+50ms;
                auto point=[](const auto& o) { return json{{"x",o.get_pos().x+o.get_size().x/2},{"y",o.get_pos().y+o.get_size().y/2}}; };
                storage_atomic_write("ui.json",json{{"dialog",bool(persistent.m_dialog)},
                    {"values",persistent.m_dialog ? persistent.m_dialog->values() : std::vector<std::string>{}},
                    {"batch",loads.m_settings.batch_request},{"workers",loads.m_settings.worker_count},{"pool",persistent.m_easy_status.size()},
                    {"width",engine.get_window_size().x},{"height",engine.get_window_size().y},
                    {"page",persistent.m_diagnostics_page},{"next",point(*persistent.m_diagnostics_next)}}.dump());
            }
            check(storage_settings_t::load().batch_request && storage_settings_t::load().worker_count==17,"Mode or worker count was not persisted.");
            std::println("PASS: worker validation, Cancel/OK, persistent independent worker count and batch mode, F3 pagination.");
        }
        else
        {
            auto control=std::make_shared<request_control_t>(); control->retry_limit=3;
            std::size_t completions{};
            auto journal=task_journal_t::create(json{{"kind","batch-mode-test"}});
            std::map<std::string,std::string> cache_keys;
            auto submit=[&](std::string label,std::size_t count)
            {
                google_batch_t request{std::move(persistent.m_easy_idle.back())};
                persistent.m_easy_idle.pop_back();
                request.clear(); request.control_set(control); request.m_journal=journal; request.path_set("/drive/v3");
                for (std::size_t i{};i<count;++i)
                {
                    auto& part=request.request_add_handle(); part.method_set("GET");
                    part.path_set("/fixture/held/"+label+"/"+std::to_string(i));
                }
                request.method_post(); request.replay(); cache_keys[label]=request.checkpoint_key();
                const auto key=request.interface_get();
                persistent.m_easy_status.at(key).m_add_time=std::chrono::steady_clock::now();
                loads.m_end.emplace(key,[&,count](auto& result)
                {
                    auto& batch=dynamic_cast<google_batch_t&>(*result);
                    check(batch.part_count()==count,"Grouped completion lost responses.");
                    for (std::size_t i{};i<count;++i)
                        check(json::parse(batch.response_get(i).body_get()).at("index")==i,"Direct responses were paired incorrectly.");
                    ++completions;
                });
                task_batch_submit(loads,multi,std::move(request));
            };
            loads.m_settings.batch_request=false; loads.m_settings.worker_count=6400; pump();
            check(persistent.m_easy_status.size()==6400,"Direct mode did not create 6400 slots.");
            submit("direct-a",100); submit("direct-b",100);
            loads.m_settings.worker_count=7;
            wait([&] { return loads.m_end.size()==7; });
            for (int i{};i<10;++i) pump();
            check(loads.m_end.size()==7,"Custom worker limit did not bound direct dispatch.");
            loads.m_settings.worker_count=200;
            wait([&] { return loads.m_pending_parts.empty(); });
            wait([&] { return api("/fixture/stats").at("held")==200; });
            check(loads.m_end.size()==200,"Direct groups did not occupy independent slots above 64.");
            const auto keys=std::views::keys(loads.m_end)|std::ranges::to<std::set>();
            loads.m_settings.worker_count=5;
            for (int i{};i<10;++i) pump();
            check((std::views::keys(loads.m_end)|std::ranges::to<std::set>())==keys,"Reducing worker count changed in-flight requests.");
            loads.m_settings.batch_request=true;
            for (int i{};i<10;++i) pump();
            check((std::views::keys(loads.m_end)|std::ranges::to<std::set>())==keys,"Switching mode changed in-flight workers.");
            check(!control->cancelled && !control->paused && !control->user_paused,"Switching mode interrupted a task.");
            api("/fixture/release-direct"); wait([&] { return completions==2; });
            submit("batch",100);
            wait([&] { return api("/fixture/stats").at("batches")==1; });
            loads.m_settings.batch_request=false; loads.m_settings.worker_count=3;
            for (int i{};i<10;++i) pump();
            check(loads.m_end.size()==1,"An in-flight batch was split during settings change.");
            api("/fixture/release-batch"); wait([&] { return completions==3; });
            check(loads.m_pending_parts.empty() && loads.m_end.empty(),"Completed groups retained slots.");
            // Pause children independently, then resume under the reduced limit.
            loads.m_settings.worker_count=100;
            api("/fixture/reset-direct"); submit("pause",100);
            wait([&] { return loads.m_pending_parts.empty(); });
            control->pause(true); wait([&] { return loads.m_end.empty(); });
            check(loads.m_parked.size()==100,"Pause lost direct child requests.");
            loads.m_settings.worker_count=13; control->pause(false);
            wait([&] { return loads.m_end.size()==13; });
            for (int i{};i<10;++i) pump();
            check(loads.m_end.size()==13,"Count-only reduction dispatched more than 13 resumed requests.");
            loads.m_settings.worker_count=64;
            wait([&] { return loads.m_end.size()==64; });
            for (int i{};i<10;++i) pump();
            check(loads.m_end.size()==64,"Reduced mode dispatched more than 64 resumed workers.");
            api("/fixture/release-direct"); wait([&] { return completions==4; });
            // Preserve a successful response that was queued just before Pause.
            loads.m_settings.batch_request=false; submit("pause-race",1);
            while (!multi.ready_nonempty()) { multi.run(); std::this_thread::sleep_for(1ms); }
            control->pause(true); wait([&] { return loads.m_end.empty(); });
            check(loads.m_parked.size()==1 && !loads.m_parked.front().request->interface_get(),"Completed paused child retained a transport.");
            control->pause(false); wait([&] { return completions==5; });
            check(loads.m_parked.empty(),"Completed paused child did not resume.");
            // Rebuild the logical group from its individual durable responses.
            // Remove its aggregate response so successful children must be used.
            std::filesystem::remove(journal->path()/(cache_keys.at("direct-a")+".response"));
            journal.reset(); journal=task_journal_t::restore().at(0);
            submit("direct-a",100); wait([&] { return completions==6; });
            journal->finish();
            std::println("PASS: 6400-slot pool, 200 simultaneous singles, live count increase/reduction, independent mode switches, indexed responses, partial retry, child checkpoint replay, pause/resume, completed pause race and reduced 64-slot admission.");
        }
        multi.stop(); engine.flush(); gui.clear_state(); lgo::log_file("finished_all!\n");
    }
    catch (const std::exception& error) { std::println("FAIL: {}",error.what()); return 1; }
}
