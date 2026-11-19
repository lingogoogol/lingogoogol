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
int main(int argc,char** argv)
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
        auto toolbar_copy_enabled=[&]{return persistent.m_folder_copy->enabled();};
        auto verify_action_layout=[&](std::string row,bool update)
        {
            lgo::scroll_t* viewer{};
            std::vector<lgo::std_liney_t*> navigation_lines;
            std::function<void(lgo::GUI_object_t&)> inspect=[&](auto& object)
            {
                if (auto scroll=dynamic_cast<lgo::scroll_t*>(&object);
                    scroll && std::dynamic_pointer_cast<lgo::divy_flex_soft_t>(scroll->content())) viewer=scroll;
                if (auto div=dynamic_cast<lgo::divx_soft_t*>(&object))
                {
                    bool navigation{};
                    std::vector<lgo::std_liney_t*> lines;
                    div->visit_children([&](auto& child)
                    {
                        if (&child==persistent.m_folder_update.get()) navigation=true;
                        if (auto line=dynamic_cast<lgo::std_liney_t*>(&child)) lines.push_back(line);
                    });
                    if (navigation) navigation_lines=std::move(lines);
                }
                object.visit_children(inspect);
            };
            inspect(*gui.get_object_shared<lgo::divy_hard_t>(0));
            check(viewer && navigation_lines.size()==2,"Missing file viewport or navigation separators");
            const auto right=viewer->get_pos().x+viewer->view_size().x;
            const auto stride=32+2*lgo::std_margin.x+lgo::std_line_size.x;
            const auto actions=std::array{"刪除","改名","複製","剪下","下載","更新"};
            const auto y=row_button(row,"下載")->get_pos().y;
            const auto* update_button=row_button(row,"更新");
            check(update_button->enabled()==update,"Update availability does not match the format version");
            if (!update) check(update_button->tooltip_text().contains("已是最新格式"),"Disabled update has no current-format explanation");
            for (std::size_t i{};i<6u;++i)
            {
                const auto* button=row_button(row,actions[i]);
                check(button->get_size().x==32 && button->get_pos().x==right-32-std::int64_t(i)*stride &&
                    button->get_pos().y==y,"File actions are shifted, reordered or clipped");
            }
            const auto buttons=std::array{persistent.m_folder_update,persistent.m_folder_download,persistent.m_folder_copy};
            for(std::size_t i{};i<2;++i)
            {
                const auto* line=navigation_lines[i];
                check(line->get_size().x==lgo::std_line_size.x &&
                    line->get_pos().x==buttons[i]->get_pos().x+32+lgo::std_margin.x &&
                    line->get_pos().x+line->get_size().x+lgo::std_margin.x==buttons[i+1]->get_pos().x &&
                    line->get_pos().y==buttons[i]->get_pos().y,
                    "Navigation separators are not centered between aligned buttons");
            }
            auto points=json::array();
            for(const auto* line : navigation_lines)
                points.push_back({line->get_pos().x,line->get_pos().y+line->get_size().y/2});
            input(json::array({json::array({"move",100,400}),json::array({"capture","layout-"+row+"-"+std::to_string(engine.get_window_size().x),
                0,0,engine.get_window_size().x,engine.get_window_size().y,points})}));
        };

        if (argc>1 && std::string_view{argv[1]}=="resume-upgrade")
        {
            restore_tasks(&loads,[&](const json&){return session;});
            wait([&]{return !loads.m_task_list.empty();});
            finish(loads.m_task_list.back());idle();
            check(name("root").at("version_min")==2,"Restored upgrade did not finish root metadata");
            begin_task(&loads,session,"download-file","large-after-replay.bin","large-file");finish(loads.m_task_list.back());
            check(storage_read("large-after-replay.bin")==storage_read("expected-large.bin"),"Crash replay changed legacy bytes");
            lgo::log_file("finished_all!\n");
            std::cout<<"PASS: interrupted upgrade restored after main publication and partial cleanup.\n";
            return 0;
        }
        if (argc>1 && std::string_view{argv[1]}=="interrupt-upgrade")
        {
            click(*persistent.m_folder_update);
            wait([&]{return false;});
        }
        operation("directory-create","root",json::object(),{},"layout-current");idle();
        verify_action_layout("old",true);
        verify_action_layout("layout-current",false);
        input(json::array({json::array({"move",100,400}),json::array({"capture","layout-after",0,0,1536,1024})}));
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
        const auto row_tasks=loads.m_task_list.size();
        hover(*row_button("layout-current","更新"),"此資料夾及其子內容已是最新格式");
        click(*row_button("layout-current","更新"));
        check(loads.m_task_list.size()==row_tasks,"Disabled folder update started a task");
        const auto paste_hint=hover(*persistent.m_paste_button,"請先剪下或複製");
        check(paste_hint.text.contains("不可用") && paste_hint.pos.y>persistent.m_paste_button->get_pos().y,"Disabled tooltip missing reason or below placement");
        input(json::array({json::array({"capture","disabled",paste_hint.pos.x,paste_hint.pos.y,paste_hint.size.x,paste_hint.size.y})}));
        click(*persistent.m_paste_button);
        check(!persistent.m_dialog && !persistent.m_clipboard,"Disabled tooltip made paste clickable");
        hover(*persistent.m_folder_copy,"根目錄無法複製");
        for (const auto action : {"更新","下載","剪下","複製","改名","刪除"})
        {
            const auto* button = row_button("old",action);
            hover(*button,button->tooltip_text());
        }
        loads.m_session.reset();persistent.m_refresh();for(int i=0;i<3;++i)pump();
        hover(*persistent.m_account_buttons.front(),"請先登入空間");
        hover(*persistent.m_folder_download,"請先登入空間");
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
        check(!toolbar_copy_enabled(),"Root copy must remain disabled under existing paste rules");
        check(persistent.m_folder_update->enabled(),"Legacy root update is disabled");
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
        check(toolbar_copy_enabled(),"Current folder copy is disabled");
        click(*persistent.m_folder_copy);
        check(persistent.m_clipboard && persistent.m_clipboard->source=="root" && persistent.m_clipboard->entity.at("id")=="old", "Navigation copy targeted the wrong folder");
        click(*persistent.m_folder_download);
        wait([&]{return !loads.m_task_list.empty() && loads.m_task_list.back()->m_type=="download";});
        finish(loads.m_task_list.back());
        check(storage_read("output/renamed/old.bin")=="old","Navigation download used the wrong folder");
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
        check(name("root").at("size")==3793753 && name("root").at("version_min")==0 && name("root").contains("time"),"Root aggregate or upgrade time incorrect");
        operation("directory-create","root",json::object(),{},"created");
        const auto created=find("root","created").at("id").get<std::string>();
        auto old=find("old","old.bin");
        operation("directory-copy","old",old,created,"old.bin");
        auto copied=find(created,"old.bin");
        check(name(created).at("version_min")==1 && name(created).at("size")==3 && name("root").at("size")==3793756,"Paste did not update ancestors");
        operation("directory-move","root",uploaded,"dest","input");
        check(name("dest").at("size")==1900000 && name(uploaded_id).at("time")==upload_time && name("root").at("size")==3793756,"Move changed time or totals");
        // Follow real uploaded indices; decode version 2 through the production downloader.
        auto nested=find(uploaded_id,"nested").at("id").get<std::string>();
        auto payload=find(nested,"payload.bin");
        begin_task(&loads,session,"download-file","download.bin",payload.at("id"));finish(loads.m_task_list.back());
        check(storage_read("download.bin")==storage_read("input/nested/payload.bin"),"Version 2 downloaded bytes differ");
        begin_task(&loads,session,"download-file","legacy.bin",copied.at("id"));finish(loads.m_task_list.back());
        check(storage_read("legacy.bin")=="old","Legacy copied download differs");
        // Row update must convert the selected file and refresh ancestor minima.
        pos=created;persistent.m_refresh();idle();
        click(*row_button("old.bin","更新"));
        wait([&]{return loads.m_task_list.back()->m_label.starts_with("更新：");});
        finish(loads.m_task_list.back());idle();
        check(name(copied.at("id")).at("version_min")==2,"Row update did not convert the file");
        verify_action_layout("old.bin",false);
        hover(*row_button("old.bin","更新"),"此檔案已是最新格式");
        const auto file_tasks=loads.m_task_list.size();
        click(*row_button("old.bin","更新"));
        check(loads.m_task_list.size()==file_tasks,"Disabled file update started a task");
        begin_task(&loads,session,"download-file","upgraded.bin",copied.at("id"));finish(loads.m_task_list.back());
        check(storage_read("upgraded.bin")=="old","Updated file bytes changed");
        // Navigation update recursively covers versions 0/1, empty files,
        // continuation pages and incompressible legacy 18,750-byte slices.
        pos="root";persistent.m_refresh();idle();
        click(*persistent.m_folder_update);
        wait([&]{return loads.m_task_list.back()->m_label.starts_with("更新：") && loads.m_task_list.back()->m_result==task_result_t::active;});
        finish(loads.m_task_list.back());idle();
        check(name("root").at("version_min")==2 && !persistent.m_folder_update->enabled(),"Root upgrade or button availability failed");
        hover(*persistent.m_folder_update,"已是最新格式");
        begin_task(&loads,session,"download-file","large-upgraded.bin","large-file");finish(loads.m_task_list.back());
        check(storage_read("large-upgraded.bin")==storage_read("expected-large.bin"),"Repartitioned legacy bytes changed");
        check(name("large-file").at("time")==123 && name("large-file").at("name")=="large.bin","Upgrade changed existing time or name");
        click(*persistent.m_folder_download);
        wait([&]{return loads.m_task_list.back()->m_type=="download";});
        finish(loads.m_task_list.back());
        check(storage_read("output/fixture/large/large.bin")==storage_read("expected-large.bin"),"Root navigation download failed");
        operation("directory-delete","root",find("root","large"),{},"large");
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
        for (const auto width : {800,1000,1536})
        {
            glfwSetWindowSize(engine.window_get(),width,800);for(int i=0;i<10;++i)pump();
            const auto update=persistent.m_folder_update->get_pos();
            const auto download=persistent.m_folder_download->get_pos();
            const auto copy=persistent.m_folder_copy->get_pos();
            check(update.x<download.x && download.x<copy.x && copy.x+32<=engine.get_window_size().x,
                "Navigation actions overlap or exceed the resized viewport");
            check(persistent.m_breadcrumbs->get_pos().x+persistent.m_breadcrumbs->get_size().x<=update.x,
                "Breadcrumbs overlap current folder actions");
            const auto* action=row_button("input","刪除");
            check(action->get_pos().x+action->get_size().x<=engine.get_window_size().x,"Row actions exceed narrow viewport");
            verify_action_layout("input",false);
        }
        engine.flush();
        std::cout<<"PASS: aligned legacy/current file actions and navigation separators across viewport sizes, rendered button tooltips, disabled reasons and click suppression, stationary updates, wrapping, edge placement, modal occlusion, version 2 transfers, GUI rename/cancel/validation, row and root upgrades, 18,750-byte repartitioning, preserved timestamps, navigation copy, and compare.\n";
        lgo::log_file("finished_all!\n");
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
