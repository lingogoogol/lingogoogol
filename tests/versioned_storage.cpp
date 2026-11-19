#include <nlohmann/json.hpp>
#include "storage/blob_format.hpp"
import std;
import lgo.io;
import lgo.math;
import lgo.GUI;
import external.GLFW;
import storage.http;
import storage.google;
import storage.kernel;
using json=nlohmann::json;
using namespace std::chrono_literals;
int main()
{
    try
    {
        auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error{text};};
        lgo::init_logfile("logfile/versioned-test.txt");
        lgo::FreeType_t freetype;
        lgo::font_face_t::freetype_set(&freetype);
        lgo::font_face_t face{"font/NotoSansTC-VariableFont_wght.ttf",16};
        lgo::font_preference_t::default_get().assign(1,&face);
        lgo::engine_t engine{lgo::size_2D{1536,1024},"Versioned storage regression"};
        struct cleanup_t{lgo::engine_t& engine;~cleanup_t(){engine.flush();lgo::text_line_t::free();}} cleanup{engine};
        lgo::depth_tracker_t depth;
        lgo::state_t gui{&engine,&depth};
        std::atomic_bool close{true}; // Never refill reserves or contact Google.
        auto session=std::make_shared<storage_session_t>();
        session->account=std::make_shared<google_account_t>();
        session->account_path="unused";session->address="fixture";session->storage_id="fixture";
        session->root="root";session->root_path="root/fixture";session->root_account="fixture";
        session->accounts["fixture"]=session->account;
        session->queue=std::make_shared<blob_queue_t>("queue",session->account.get(),&close);
        session->queues["fixture"]=session->queue;
        session->remember("root","fixture","folder");
        google_account_t::default_shared_set(session->account);
        load_state_t loads;loads.m_session=session;loads.m_settings.network_retries=0;
        curl_multi_worker_t multi;
        GUI_persistent_t persistent;
        std::shared_ptr<lgo::divy_soft_t> column;
        std::string pos="root";
        home(&engine,&gui,&persistent,&column,&loads,&multi,session->queue.get(),&pos);
        auto pump=[&]{check(!message_loop(&engine,&persistent,&multi,&loads,session->queue.get()),"Window closed");lgo::text_line_t::pending_load();std::this_thread::sleep_for(1ms);};
        auto wait=[&](auto done){const auto deadline=std::chrono::steady_clock::now()+45s;while(!done()){check(std::chrono::steady_clock::now()<deadline,"Timed out");pump();}};
        wait([&]{return persistent.m_listings.empty();});
        // Deliberately block the journal writer: input/rendering and enqueueing
        // a new intent/pause must remain independent of a slow disk.
        for(int i=0;i<20;++i)pump();
        storage_write_queue_t disk;
        auto started=std::make_shared<std::atomic_bool>(false);
        disk.push([started]{*started=true;std::this_thread::sleep_for(350ms);});
        wait([&]{return started->load();});
        const auto queued=std::chrono::steady_clock::now();
        auto intent=task_journal_t::create({{"kind","upload-file"},{"path","latency-fixture"}});
        intent->set("user_paused",true);
        check(std::chrono::steady_clock::now()-queued<150ms,"GUI waited for blocked journal I/O");
        check(!intent->ready(),"Intent was sent past a blocked durability barrier");
        unsigned responsive_frames{};
        while(!intent->ready()){pump();++responsive_frames;}
        check(responsive_frames>=2,"Render loop did not continue while saving state");
        intent->finish();wait([&]{return intent->ready();});
        google_easy_t request;
        auto entries=[&](std::string id){return list_folder_content(request,&id,session.get());};
        auto name=[&](const std::string& id){session->bind(request,id);return json::parse(blob_get_name_run(request,id));};
        auto find=[&](std::string parent,std::string text){for(auto& e:entries(parent))if(e.at("name")==text)return e;throw std::runtime_error{"Missing entry: "+text};};
        auto finish=[&](const std::shared_ptr<load_task_t>& task){unsigned retries=0;wait([&]{if(task->m_control->paused){if(++retries>3)throw std::runtime_error{task->m_control->reason()};task->m_control->retry();}return task->m_result!=task_result_t::active;});check(task->m_result==task_result_t::completed,"Task failed");};
        auto operation=[&](std::string kind,std::string source,json entry,std::string destination,std::string text){begin_directory_task(&loads,session,kind,source,entry,destination,text);finish(loads.m_task_list.back());};
        unsigned step{};
        auto input=[&](json commands)
        {
            storage_atomic_write("ui-step.json",json{{"step",++step},{"commands",commands}}.dump());
            wait([&]{return std::filesystem::exists("ui-ack") && storage_read("ui-ack")==std::to_string(step);});
        };
        auto click=[&](const lgo::GUI_object_t& object)
        {
            const auto p=object.get_pos();const auto n=object.get_size();
            input(json::array({json::array({"click",p.x+n.x/2,p.y+n.y/2})}));
        };
        auto row_button=[&](std::string text,std::string action)->progress_button_t*
        {
            std::optional<std::int64_t> y;
            std::vector<progress_button_t*> buttons;
            std::function<void(lgo::GUI_object_t&)> scan=[&](auto& object)
            {
                if (auto label=dynamic_cast<lgo::button_text_t<lgo::text_line_scroll_std_t>*>(&object);
                    label && label->get_text()==text) y=label->get_pos().y;
                if (auto button=dynamic_cast<progress_button_t*>(&object);button && button->get_text()==action) buttons.push_back(button);
                object.visit_children(scan);
            };
            scan(*gui.get_object_shared<lgo::divy_hard_t>(0));
            for (auto* button:buttons) if (y && button->get_pos().y==*y) return button;
            throw std::runtime_error{"Missing row action: "+text+" / "+action};
        };
        auto idle=[&]{wait([&]{return persistent.m_listings.empty();});};
        auto tips=lgo::button_tooltip_t::shared(&engine);
        auto hover=[&](const lgo::GUI_object_t& object,std::string_view expected)
        {
            const auto at=object.get_pos();const auto size=object.get_size();
            input(json::array({json::array({"move",at.x+size.x/2,at.y+size.y/2})}));
            wait([&]{auto hint=tips->snapshot();return hint && hint->text.contains(expected);});
            const auto hint=tips->snapshot().value();
            check(hint.pos.y>=at.y+size.y || hint.pos.y+hint.size.y<=at.y,"Tooltip overlaps its button");
            check(hint.pos.x>=0 && hint.pos.x+hint.size.x<=engine.get_window_size().x,"Tooltip exceeds horizontal viewport");
            return hint;
        };
        const auto paste_hint=hover(*persistent.m_paste_button,"請先剪下或複製");
        check(paste_hint.text.contains("不可用") && paste_hint.pos.y>persistent.m_paste_button->get_pos().y,"Disabled tooltip missing reason or below placement");
        input(json::array({json::array({"capture","disabled",paste_hint.pos.x,paste_hint.pos.y,paste_hint.size.x,paste_hint.size.y})}));
        click(*persistent.m_paste_button);
        check(!persistent.m_dialog && !persistent.m_clipboard,"Disabled tooltip made paste clickable");
        for (const auto action : {"下載","剪下","複製","刪除"})
            hover(*row_button("old",action),row_button("old",action)->tooltip_text());
        loads.m_session.reset();persistent.m_refresh();for(int i=0;i<3;++i)pump();
        hover(*persistent.m_account_buttons.front(),"請先登入空間");
        loads.m_session=session;persistent.m_refresh();idle();
        // A long tooltip wraps by code point and can update while the cursor
        // remains stationary. Disabling under hover must suppress the action.
        unsigned clicked{};
        auto fixture=gui.add_object_shared<progress_button_t>(lgo::depth_range_t{.008f,.009f},
            lgo::pos_2D{1400,960},[&]{++clicked;},"tooltip fixture",lgo::std_white,lgo::size_1D{100});
        const std::string long_hint="長文字換行測試："+std::string(180,'W');
        fixture->set_tooltip(long_hint);
        const auto wrapped=hover(*fixture,long_hint);
        check(wrapped.size.y>40 && wrapped.pos.y+wrapped.size.y<fixture->get_pos().y,"Long tooltip did not wrap or fall back above");
        fixture->set_enabled(false,"測試中，暫時無法操作");
        wait([&]{auto hint=tips->snapshot();return hint && hint->text.contains("測試中，暫時無法操作");});
        click(*fixture);check(clicked==0,"Disabled button invoked its action");
        fixture->set_enabled(true);hover(*fixture,long_hint);click(*fixture);
        check(clicked==1,"Enabled button did not invoke exactly once");
        fixture->hide();wait([&]{return !tips->snapshot();});
        fixture.reset();
        const auto rename_before=name("old-file");
        const auto tasks_before=loads.m_task_list.size();
        click(*row_button("old","改名"));
        wait([&]{return bool(persistent.m_dialog);});
        input(json::array({json::array({"move",persistent.m_paste_button->get_pos().x+20,persistent.m_paste_button->get_pos().y+10})}));
        for(int i=0;i<5;++i)pump();
        check(!tips->snapshot(),"Background tooltip leaked through modal overlay");
        // Cancel must not schedule any writes.
        const auto dialog_top=(engine.get_window_size().y-220)/2;
        input(json::array({json::array({"move",(engine.get_window_size().x-720)/2+520,dialog_top+180})}));
        wait([&]{auto hint=tips->snapshot();return hint && hint->text.contains("取消本次操作");});
        input(json::array({json::array({"click",(engine.get_window_size().x-720)/2+520,dialog_top+180})}));
        wait([&]{return !persistent.m_dialog;});
        check(loads.m_task_list.size()==tasks_before,"Cancelled rename started a task");
        click(*row_button("old","改名"));wait([&]{return bool(persistent.m_dialog);});
        const auto left=(engine.get_window_size().x-720)/2;
        input(json::array({json::array({"click",left+180,dialog_top+90}),json::array({"key","ctrl+a"}),
            json::array({"type","/"}),json::array({"key","Return"})}));
        check(bool(persistent.m_dialog) && loads.m_task_list.size()==tasks_before,"Invalid rename was accepted");
        input(json::array({json::array({"key","ctrl+a"}),json::array({"type","renamed"}),json::array({"key","Return"})}));
        wait([&]{return !persistent.m_dialog && loads.m_task_list.size()>tasks_before;});
        finish(loads.m_task_list.back());idle();
        check(find("root","renamed").at("id")=="old" && name("old").at("name")=="renamed", "Folder rename lost ID or index name");
        check(name("old-file")==rename_before,"Folder rename changed child metadata");
        pos="old";persistent.m_refresh();idle();
        // File rename also updates both the directory entry and blob metadata.
        operation("directory-rename","old",find("old","old.bin"),{},"renamed.bin");idle();
        check(name("old-file").at("name")=="renamed.bin","File rename missed metadata");
        operation("directory-rename","old",find("old","renamed.bin"),{},"old.bin");idle();
        operation("directory-rename","root",find("root","renamed"),{},"old");idle();
        pos="root";persistent.m_refresh();idle();
        begin_task(&loads,session,"upload-folder","input","root");finish(loads.m_task_list.back());
        auto uploaded=find("root","input");const auto uploaded_id=uploaded.at("id").get<std::string>();
        auto metadata=name(uploaded_id);
        check(metadata.at("version")==2 && metadata.at("version_min")==2 && metadata.at("size")==1900000 && metadata.at("time").get<std::int64_t>()>0,"Uploaded folder metadata incorrect");
        const auto upload_time=metadata.at("time");
        check(name("root").at("size")==1900003 && name("root").at("version_min")==0 && name("root").contains("time"),"Root aggregate or upgrade time incorrect");
        operation("directory-create","root",json::object(),{},"created");
        const auto created=find("root","created").at("id").get<std::string>();
        auto old=find("old","old.bin");
        operation("directory-copy","old",old,created,"old.bin");
        auto copied=find(created,"old.bin");
        check(name(created).at("version_min")==0 && name(created).at("size")==3 && name("root").at("size")==1900006,"Paste did not update ancestors");
        operation("directory-move","root",uploaded,"dest","input");
        check(name("dest").at("size")==1900000 && name(uploaded_id).at("time")==upload_time && name("root").at("size")==1900006,"Move changed time or totals");
        // Follow real uploaded indices; decode version 2 through the production downloader.
        auto nested=find(uploaded_id,"nested").at("id").get<std::string>();
        auto payload=find(nested,"payload.bin");
        begin_task(&loads,session,"download-file","download.bin",payload.at("id"));finish(loads.m_task_list.back());
        check(storage_read("download.bin")==storage_read("input/nested/payload.bin"),"Version 2 downloaded bytes differ");
        begin_task(&loads,session,"download-file","legacy.bin",copied.at("id"));finish(loads.m_task_list.back());
        check(storage_read("legacy.bin")=="old","Legacy copied download differs");
        operation("directory-delete","root",find("root","old"),{},"old");
        operation("directory-delete",created,copied,{},"old.bin");
        check(name("root").at("size")==1900000 && name("root").at("version_min")==2 && name(created).at("size")==0,"Deletion did not update all ancestors");
        pos="dest";persistent.m_refresh();wait([&]{return persistent.m_listings.empty();});
        check(persistent.m_directory_status->get_text().empty(),"Listing failed");
        bool heading=false, size_label=false, timestamp=false;
        std::function<void(lgo::GUI_object_t&)> inspect=[&](auto& object)
        {
            if(auto text=dynamic_cast<lgo::text_line_t*>(&object))
            {
                const auto value=text->get_text();
                heading |= value=="時間";
                size_label |= value=="1 MB";
                timestamp |= value.size()==19 && value[4]=='-' && value[7]=='-' && value[10]==' ';
            }
            object.visit_children(inspect);
        };
        inspect(*gui.get_object_shared<lgo::divy_hard_t>(0));
        check(heading && size_label && timestamp,"Missing time column, folder size or timestamp text");
        glfwSetWindowSize(engine.window_get(),1000,800);for(int i=0;i<5;++i)pump();
        engine.flush();
        std::cout<<"PASS: native button tooltip regression, version 2 folder upload/download, legacy copy/download, create/move/delete, ancestor size/version_min, upgrade and preserved timestamps, listing and resize.\n";
        lgo::log_file("finished_all!\n");
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
