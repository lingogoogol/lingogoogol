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
void check(bool ok, const char* text) { if (!ok) throw std::runtime_error{text}; }
int main(int argc,char** argv)
{
    try
    {
        check(argc==2,"Missing phase.");
        const std::string phase=argv[1];
        lgo::init_logfile("logfile/spaces.txt");
        lgo::FreeType_t freetype;
        lgo::font_face_t::freetype_set(&freetype);
        lgo::font_face_t face{"font/NotoSansTC-VariableFont_wght.ttf",16};
        lgo::font_preference_t::default_get().assign(1,&face);
        lgo::engine_t engine{lgo::size_2D{1536,1024},"Space routing regression"};
        struct cleanup_t { lgo::engine_t& engine; ~cleanup_t() { engine.flush(); lgo::text_line_t::free(); } } cleanup{engine};
        lgo::depth_tracker_t depth;
        lgo::state_t gui{&engine,&depth};
        std::atomic_bool close{};
        storage_runtime_t runtime{&close};
        auto settings=storage_settings_t::load();
        settings.network_retries=3;
        const auto registered=storage_service_accounts();
        check(registered.size()==4,"External key/ files were treated as program-managed accounts.");
        auto session=runtime.open_space(settings,settings.spaces.at(0),std::make_shared<request_control_t>());
        check(session->accounts.size()==4 && session->queues.size()==4,"Space lost an identity or per-account queue.");
        auto peer=runtime.open_space(settings,settings.spaces.at(1),std::make_shared<request_control_t>());
        const auto shared_account=peer->accounts.begin()->first;
        check(peer->queues.at(shared_account)==session->queues.at(shared_account),"Shared account has two reserve queues.");
        runtime.sign_out(peer->storage_id);
        if (!session->clearing) session->queues.at(shared_account)->get(1);
        peer=runtime.open_space(settings,settings.spaces.at(1),std::make_shared<request_control_t>());
        check(peer->queues.at(shared_account)==session->queues.at(shared_account),"Signing out one space replaced another space's queue.");
        std::map<std::string,int> distribution;
        std::string previous_project;
        for (int i{};i<400;++i)
        {
            const auto address=session->choose(100);
            ++distribution[address];
            const auto project=address.substr(address.find('@')+1);
            check(project!=previous_project,"Allocation exhausted one project before visiting the other.");
            previous_project=project;
        }
        for (const auto& [address,count] : distribution) check(count==100,"Service account allocation is unbalanced.");
        load_state_t loads;
        loads.m_session=session; loads.m_settings=settings;
        curl_multi_worker_t multi;
        GUI_persistent_t persistent;
        struct shutdown_t
        {
            std::atomic_bool& close; storage_runtime_t& runtime; load_state_t& loads; curl_multi_worker_t& multi;
            ~shutdown_t() { close=true; for (auto& task:loads.m_task_list) task->m_control->cancel(); runtime.stop(); multi.stop(); }
        } shutdown{close,runtime,loads,multi};
        std::string pos=session->root;
        std::shared_ptr<lgo::divy_soft_t> column;
        home(&engine,&gui,&persistent,&column,&loads,&multi,session->queue.get(),&pos);
        auto pump=[&]
        {
            check(!message_loop(&engine,&persistent,&multi,&loads,session->queue.get()),"Window closed early.");
            lgo::text_line_t::pending_load();
            check(persistent.m_easy_idle.size()+loads.m_end.size()==transfer_slot_count,"A request escaped the shared transfer pool.");
            for (const auto& task:loads.m_task_list)
                if (task->m_control->paused) throw std::runtime_error{task->m_control->reason()};
        };
        auto wait=[&](auto done)
        {
            const auto deadline=std::chrono::steady_clock::now()+90s;
            while (!done()) { check(std::chrono::steady_clock::now()<deadline,"Space operation timed out."); pump(); }
        };
        auto settle=[&]
        {
            wait([&] { return loads.m_task_list.empty(); });
            for (const auto& task:loads.m_finished_tasks)
                if (task->m_result!=task_result_t::completed) throw std::runtime_error{task->m_progress_text->get_text()};
            for (auto& task:loads.m_finished_tasks) task->m_dismissed=true;
            pump();
        };
        auto entries=[&](std::string id)
        { google_easy_t request; return list_folder_content(request,&id,session.get()); };
        auto validate=[&]
        {
            google_easy_t request;
            request.path_set("/fixture/validate"); request.method_get(); request.run();
        };
        if (phase=="upload-crash")
        {
            begin_task(&loads,session,"upload-folder","input/tree",pos);
            auto task=std::dynamic_pointer_cast<upload_task_t>(loads.m_task_list.back());
            wait([&] { return task->m_progress>2000000; });
            std::_Exit(0); // Exercise the real durable account/ID reservations.
        }
        if (phase!="clear-resume")
        {
        restore_tasks(&loads,[&](const json&) { return session; });
        check(!loads.m_task_list.empty(),"Interrupted upload was not restored.");
        settle();
        auto all=entries(pos);
        validate();
        auto tree=*std::ranges::find_if(all,[](const auto& entry) { return entry.at("name")=="tree"; });
        const auto large=*std::ranges::find_if(all,[](const auto& entry) { return entry.at("name")=="large-index"; });
        begin_task(&loads,session,"download-folder","roundtrip",tree.at("id")); settle();
        check(storage_read("roundtrip/payload.bin")==storage_read("input/tree/payload.bin"),"Cross-account download differs.");
        begin_directory_task(&loads,session,"directory-create",pos,{}, {},"destination"); settle();
        all=entries(pos);
        const auto destination=*std::ranges::find_if(all,[](const auto& entry) { return entry.at("name")=="destination"; });
        begin_directory_task(&loads,session,"directory-copy",pos,tree,destination.at("id"),"tree"); settle();
        validate();
        begin_directory_task(&loads,session,"directory-delete",pos,tree,{},"tree"); settle();
        const auto copied=entries(destination.at("id")).at(0);
        begin_task(&loads,session,"download-folder","copied",copied.at("id")); settle();
        check(storage_read("copied/payload.bin")==storage_read("input/tree/payload.bin"),"Independent copied data differs after source deletion.");
        // Move the surviving tree back to root, updating account-qualified parents.
        begin_directory_task(&loads,session,"directory-move",destination.at("id"),copied,pos,"tree"); settle();
        begin_directory_task(&loads,session,"directory-copy",pos,large,destination.at("id"),"large-index"); settle();
        validate();
        begin_directory_task(&loads,session,"directory-delete",pos,large,{},"large-index"); settle();
        }
        auto control=std::make_shared<request_control_t>(); control->retry_limit=3;
        std::atomic_uint64_t progress{},total{};
        session->clearing=true;
        auto clear=std::async(std::launch::async,[&] { return runtime.clear(session,control,progress,total); });
        if (phase=="resume")
        {
            wait([&] { return progress>0; });
            std::_Exit(0); // Clear resumes from its complete account-qualified inventory.
        }
        wait([&] { return clear.wait_for(0s)==std::future_status::ready; });
        runtime.cleared(session,clear.get());
        check(peer->queues.at(shared_account)==session->queues.at(shared_account),"Clear split a shared account's reserve queues.");
        check(progress==total && entries(session->root).empty(),"Space clear did not finish.");
        const auto root=json::parse(storage_read(session->root_path));
        check(root.at("id")==session->root && root.at("account")==session->root_account,"Root identity was not saved together.");
        std::println("PASS: managed identities, interleaved projects/users, crash replay, cross-account upload/download/copy/move/delete, space clear.");
        engine.flush(); gui.clear_state();
    }
    catch (const std::exception& error) { std::println("FAIL: {}",error.what()); return 1; }
    lgo::log_file("finished_all!\n");
}
