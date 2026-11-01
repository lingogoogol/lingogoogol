#include <nlohmann/json.hpp>

import std;

import external.Windows;
import external.Vulkan;
import external.GLFW;
import lgo.container.str;
import external.processenv;
import external.shellapi;

import lgo.math;
import lgo.io;
import lgo.GUI;

import storage.http;
import storage.google;
import storage.kernel;

std::atomic_bool close_app{false};

auto app() -> void
{
    // Explorer may start with bin as CWD. Packaged GUI resources and state
    // always belong beside bin; command-line helper tools retain their CWD.
    std::vector<wchar_t> executable_name(32768);
    const auto length = GetModuleFileNameW(nullptr,executable_name.data(),
        static_cast<DWORD>(executable_name.size()));
    if (!length || length==executable_name.size())
        throw std::runtime_error{"Cannot determine the executable path."};
    const std::filesystem::path executable{std::wstring{executable_name.data(),length}};
    auto directory = executable.parent_path().filename().wstring();
    std::ranges::transform(directory,directory.begin(),[](wchar_t c)
        { return c>=L'A' && c<=L'Z' ? c-L'A'+L'a' : c; });
    if (directory==L"bin") std::filesystem::current_path(executable.parent_path().parent_path());
    auto settings{storage_settings_t::load()};
    // Keep one filename for this process; ordinary settings/space operations
    // must not split the session log. A directory change retains this name.
    auto log_path = [](const storage_settings_t& value)
    {
        return (std::filesystem::path{value.logfile_path} / (std::to_string(std::time(nullptr)) + ".txt"))
            .string();
    };
    lgo::init_logfile(log_path(settings));
    http_error_log_codes_set(storage_settings_t::parse_http_log_codes(settings.http_log_codes));
    lgo::log_file("CWD: " + std::filesystem::current_path().string() + "\n");
    lgo::FreeType_t freetype;
    lgo::font_face_t::freetype_set(&freetype);
    // Visible glyphs and the GPU cache retain face pointers across live changes.
    std::vector<std::unique_ptr<lgo::font_face_t>> fonts;
    fonts.push_back(std::make_unique<lgo::font_face_t>(settings.font_path, lgo::std_font_size.x));
    lgo::font_preference_t::default_get().assign(1, fonts.back().get());
    lgo::engine_t engine{lgo::size_2D{1536, 1024}, "wWinMain.engine"};
    struct font_cleanup_t
    {
        lgo::engine_t& engine;
        ~font_cleanup_t()
        {
            try
            {
                engine.flush();
            }
            catch (...)
            {
            }
            lgo::text_line_t::free();
        }
    } font_cleanup{engine};
    lgo::depth_tracker_t depth;
    lgo::state_t state{&engine, &depth};
    storage_runtime_t runtime{&close_app};
    auto startup_control = std::make_shared<request_control_t>();
    std::shared_ptr<storage_session_t> session;
    std::string startup_error;
    for (const auto& space : settings.spaces)
        if (space.signed_in)
            try
            {
                auto restored = runtime.open_space(settings,space,startup_control);
                if (space.id==settings.selected_space) session=std::move(restored);
            }
            catch (const std::exception& error) { startup_error=error.what(); }
    if (session) google_account_t::default_shared_set(session->account);
    load_state_t load_state;
    load_state.m_session = session;
    load_state.m_settings = settings;
    curl_multi_worker_t multi;
    std::string pos{session ? session->root : ""};
    std::shared_ptr<lgo::divy_soft_t> progress_column;
    GUI_persistent_t persistent;
    struct settings_result_t
    {
        storage_settings_t settings;
        std::shared_ptr<storage_session_t> session;
    };
    std::future<settings_result_t> settings_job;
    struct cloud_result_t
    {
        enum class kind_t { login, projects, accounts, created_project, created_account, remove } kind;
        std::shared_ptr<google_oauth_user_t> user;
        std::string project;
        std::vector<cloud_project_t> projects;
        std::vector<cloud_service_account_t> accounts;
        cloud_result_t(kind_t value, std::shared_ptr<google_oauth_user_t> identity = {}, std::string id = {})
            : kind{value}, user{std::move(identity)}, project{std::move(id)} {}
    };
    std::future<cloud_result_t> cloud_job;
    auto settings_control{std::make_shared<request_control_t>()};
    std::shared_ptr<lgo::text_line_scroll_std_t> settings_text;
    std::shared_ptr<progress_button_t> settings_retry, settings_cancel, settings_confirm;
    std::shared_ptr<lgo::std_liney_t> settings_separator, clear_separator;
    std::future<std::string> clear_job;
    auto clear_control{std::make_shared<request_control_t>()};
    std::shared_ptr<storage_session_t> clearing_session;
    std::atomic_uint64_t clear_progress{}, clear_total{};
    std::shared_ptr<lgo::text_line_scroll_std_t> clear_text;
    std::shared_ptr<progress_button_t> clear_retry;
    std::shared_ptr<progress_button_t> clear_cancel, clear_confirm;
    // Completed clear rows remain independent of later clears. Input callbacks
    // only mark rows; removing their controls during input dispatch is unsafe.
    std::vector<std::pair<std::uint64_t, std::shared_ptr<bool>>> clear_rows;
    std::shared_ptr<bool> clear_dismissed;
    bool clear_restart{};
    struct shutdown_t
    {
        storage_runtime_t& runtime;
        load_state_t& tasks;
        curl_multi_worker_t& multi;
        GUI_persistent_t& gui;
        std::shared_ptr<request_control_t> settings, clear;
        ~shutdown_t()
        {
            close_app = true;
            settings->cancel();
            clear->cancel();
            for (auto& listing : gui.m_listings)
                listing.control->cancel();
            // Shutdown cancellation is deliberately not saved as user cancellation.
            // Every task's last durable checkpoint stays available for next launch.
            for (auto& task : tasks.m_task_list)
                task->m_control->cancel();
            for (auto& task : tasks.m_finished_tasks)
                task->m_control->cancel();
            runtime.stop();
            multi.stop();
        }
    } shutdown{runtime, load_state, multi, persistent, settings_control, clear_control};
    home(&engine, &state, &persistent, &progress_column, &load_state, &multi, session ? session->queue.get() : nullptr, &pos);
    std::string settings_task_name;
    std::vector<std::pair<std::uint64_t,std::shared_ptr<bool>>> settings_rows;
    bool restore_after_login{};
    auto begin_settings_row = [&](std::string name, bool cancellable)
    {
        settings_task_name = std::move(name);
        const auto row_id = progress_column->add_object<lgo::divy_flex_soft_t>(
            storage_content_depth,progress_column->space_x(lgo::std_margin),lgo::std_margin,
            lgo::alignment_2D{lgo::alignment_x::left,lgo::alignment_y::top});
        auto row = progress_column->get_object_shared<lgo::divy_flex_soft_t>(row_id);
        auto dismissed = std::make_shared<bool>(false);
        settings_rows.emplace_back(row_id,dismissed);
        settings_text = row->add_object_shared<lgo::text_line_scroll_std_t>(
            storage_content_depth, settings_task_name+"（處理中）", lgo::std_red_light, lgo::size_1D{256}, false);
        auto settings_actions{row->add_object_shared<lgo::divx_soft_t>(
            storage_content_depth, lgo::div_create_from_member{},
            lgo::size_1D{lgo::font_preference_t::default_get()[0]->leading_get()}, lgo::std_margin,
            row->space_x(lgo::std_margin),
            lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::top})};
        settings_retry = settings_actions->add_object_shared<progress_button_t>(
            storage_content_depth,
            [&]
            {
                runtime.retry_accounts();
                settings_control->retry();
            },
            "重試", lgo::std_white, lgo::size_1D{32});
        settings_retry->hide();
        settings_separator = settings_actions->add_object_shared<lgo::std_liney_t>(
            storage_content_depth, lgo::size_1D{lgo::font_preference_t::default_get()[0]->leading_get()}, lgo::std_white);
        settings_separator->hide();
        settings_cancel = settings_actions->add_object_shared<progress_button_t>(
            storage_content_depth, [&] { settings_control->cancel(); }, "取消", lgo::std_white, lgo::size_1D{32});
        settings_confirm = settings_actions->add_object_ghost_shared<progress_button_t>(
            storage_content_depth, [dismissed] { *dismissed = true; }, "確定", lgo::std_white, lgo::size_1D{32});
        settings_confirm->hide();
        // Hidden children retain slots; terminal confirmation replaces retry/cancel.
        settings_cancel->set_pos(settings_retry->get_pos());
        settings_confirm->set_pos(settings_retry->get_pos());
        if (!cancellable) settings_cancel->hide();
    };
    enum class menu_page_t { spaces, links, users, projects, services };
    auto menu_page = menu_page_t::spaces;
    std::string editing_space, selected_project;
    std::shared_ptr<google_oauth_user_t> cloud_user;
    std::vector<cloud_project_t> projects;
    std::vector<cloud_service_account_t> services;
    auto show_menu = [&] { persistent.m_open_accounts(false); };
    auto space_by_id = [&](const std::string& id) -> storage_space_t&
    {
        const auto found = std::ranges::find(settings.spaces,id,&storage_space_t::id);
        if (found==settings.spaces.end()) throw std::runtime_error{"空間已移除"};
        return *found;
    };
    auto sync_settings = [&]
    {
        settings.save();
        load_state.m_settings = settings;
    };
    auto suspend_space = [&](const std::string& id)
    {
        auto tasks = load_state.m_task_list;
        for (auto& task : tasks)
            if (task->m_session && task->m_session->storage_id==id)
                finish_task(*task,task_result_t::signed_out,"等待空間登入："+task->m_label);
        for (auto& listing : persistent.m_listings)
            if (listing.session && listing.session->storage_id==id) listing.control->cancel();
        if (persistent.m_clipboard && persistent.m_clipboard->session->storage_id==id)
            persistent.m_clipboard.reset();
        multi.interrupt();
    };
    auto start_settings = [&](storage_settings_t candidate, std::string name,
                              std::function<std::shared_ptr<storage_session_t>()> work)
    {
        if (settings_job.valid() || cloud_job.valid() || clearing_session)
            throw std::runtime_error{"另一個操作仍在執行"};
        settings_control->retry();
        settings_control->cancelled=false;
        settings_control->retry_limit=candidate.network_retries;
        begin_settings_row(std::move(name),true);
        persistent.m_switching=true;
        settings_job=std::async(std::launch::async,
            [candidate=std::move(candidate),work=std::move(work)]
            { return settings_result_t{candidate,work()}; });
    };
    persistent.m_apply_settings = [&](storage_settings_t candidate)
    {
        auto next_font=std::make_unique<lgo::font_face_t>(candidate.font_path,lgo::std_font_size.x);
        std::filesystem::create_directories(candidate.logfile_path);
        const auto probe_path=std::filesystem::path{candidate.logfile_path}/
            (".storage-write-test."+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::ofstream probe{probe_path,std::ios::binary};
        if (!probe) throw std::runtime_error{"無法寫入日誌資料夾"};
        probe.close();
        std::filesystem::remove(probe_path);
        start_settings(candidate,"更新設定",[&,candidate,current=load_state.m_session]
        {
            if (current && candidate.root_path!=settings.root_path)
                return runtime.open_space(candidate,*std::ranges::find(candidate.spaces,current->storage_id,&storage_space_t::id),settings_control);
            return current;
        });
    };
    auto sign_in = [&](std::string id)
    {
        auto candidate=settings;
        auto& space=*std::ranges::find(candidate.spaces,id,&storage_space_t::id);
        space.signed_in=true;
        candidate.selected_space=id;
        start_settings(candidate,"登入空間："+space.name,[&,candidate,space]
        {
            for (const auto& current : runtime.signed_in_sessions())
                if (current->storage_id==space.id) return current;
            return runtime.open_space(candidate,space,settings_control);
        });
    };
    auto sign_out = [&](std::string id, bool remove_space, std::string remove_account)
    {
        if (settings_job.valid() || cloud_job.valid() || clearing_session) return;
        auto candidate=settings;
        auto found=std::ranges::find(candidate.spaces,id,&storage_space_t::id);
        if (found==candidate.spaces.end()) return;
        found->signed_in=false;
        if (!remove_account.empty()) std::erase(found->accounts,remove_account);
        const auto name=found->name;
        if (remove_space) candidate.spaces.erase(found);
        auto current=load_state.m_session;
        if (current && current->storage_id==id) current.reset();
        if (candidate.selected_space==id) candidate.selected_space.clear();
        suspend_space(id);
        start_settings(candidate,remove_space ? "移除空間："+name : remove_account.empty() ? "登出空間："+name : "移除服務帳戶關聯",
            [&,candidate,current,id,remove_space]
        {
            runtime.sign_out(id);
            if (remove_space) std::filesystem::remove(std::filesystem::path{candidate.root_path}/id);
            return current;
        });
    };
    auto name_dialog = [&](std::string title, std::string label, std::function<void(std::string)> accept)
    {
        persistent.m_dialog_queue.push_back([&,title=std::move(title),label=std::move(label),accept=std::move(accept)]
        {
            persistent.m_dialog=std::make_unique<lgo::modal_dialog_t>(&engine,&depth,title,
                std::vector<std::string>{},std::vector<std::pair<std::string,std::string>>{{label,""}});
            persistent.m_dialog_done=[&,accept](bool ok,const auto& values)
            {
                if (!ok) { persistent.m_deferred.push_back(show_menu); return true; }
                try
                {
                    if (values.at(0).empty() || std::ranges::all_of(values.at(0),[](unsigned char c) { return c<=32; }))
                        throw std::runtime_error{"請輸入名稱"};
                    accept(values.at(0));
                    return true;
                }
                catch (const std::exception& error) { persistent.m_dialog->error(error.what()); return false; }
            };
        });
    };
    auto typed_removal = [&](std::string title,std::vector<std::string> consequences,std::string phrase,std::function<void()> apply)
    {
        consequences.push_back("請輸入「"+phrase+"」以確認。");
        persistent.m_dialog_queue.push_back([&,title=std::move(title),consequences=std::move(consequences),phrase=std::move(phrase),apply=std::move(apply)]
        {
            persistent.m_dialog=std::make_unique<lgo::modal_dialog_t>(&engine,&depth,title,consequences,
                std::vector<std::pair<std::string,std::string>>{{"確認文字",""}},"移除");
            persistent.m_dialog_done=[&,phrase,apply](bool ok,const auto& values)
            {
                if (!ok) { persistent.m_deferred.push_back(show_menu); return true; }
                if (values.at(0)!=phrase) { persistent.m_dialog->error("確認文字不符"); return false; }
                apply();
                return true;
            };
        });
    };
    auto begin_cloud = [&](std::string name,std::function<cloud_result_t()> work)
    {
        if (settings_job.valid() || cloud_job.valid() || clearing_session)
            throw std::runtime_error{"另一個操作仍在執行"};
        settings_control->retry();
        settings_control->cancelled=false;
        begin_settings_row(std::move(name),true);
        persistent.m_switching=true;
        cloud_job=std::async(std::launch::async,std::move(work));
    };
    auto select_project = [&](std::string project)
    {
        selected_project=project;
        begin_cloud("讀取服務帳戶",[user=cloud_user,project,control=settings_control]
        {
            cloud_result_t result{cloud_result_t::kind_t::accounts,user,project};
            result.accounts=google_cloud_accounts(user,control,project);
            return result;
        });
    };
    persistent.m_space_menu_rows = [&]
    {
        std::vector<GUI_persistent_t::menu_row_t> rows;
        if (menu_page==menu_page_t::spaces)
            for (const auto& space : settings.spaces)
                rows.push_back({space.name,{},false,{
                    {"登入",[&,id=space.id] { sign_in(id); }},
                    {"登出",[&,id=space.id] { sign_out(id,false,{}); }},
                    {"編輯",[&,id=space.id] { editing_space=id; menu_page=menu_page_t::links; show_menu(); }},
                    {"移除",[&,space]
                    {
                        typed_removal("移除空間",{"空間："+space.name,"將移除本機空間設定與 root 紀錄，並登出此空間、暫停傳輸。",
                            "所有雲端 blob 與服務帳戶金鑰都會保留。","失去 root 紀錄後，新建同名空間也不會自動找回原資料。"},
                            "移除空間",[&,id=space.id] { menu_page=menu_page_t::spaces; sign_out(id,true,{}); });
                    }}}});
        else if (menu_page==menu_page_t::links)
            for (const auto& address : space_by_id(editing_space).accounts)
                rows.push_back({address.substr(0,address.find('@')),{},false,{{"移除",[&,address]
                {
                    typed_removal("移除服務帳戶關聯",{"服務帳戶："+address,"只解除此空間的關聯，雲端 blob 與私鑰都會保留。",
                        "將登出此空間並暫停傳輸。存在此帳戶的資料及依賴其索引的內容可能無法存取。",
                        "重新關聯同一服務帳戶後可恢復存取。"},"移除關聯",[&,address] { sign_out(editing_space,false,address); });
                }}}});
        else if (menu_page==menu_page_t::users)
        {
            if (!std::filesystem::exists(settings.account_path)) return rows;
            for (const auto& user : google_oauth_user_t::saved_accounts(settings.account_path,oauth_purpose_t::cloud))
                rows.push_back({user.address,user.portrait,true,{
                    {"使用",[&,email=user.address]
                    {
                        begin_cloud("讀取專案",[path=settings.account_path,email,control=settings_control]
                        {
                            auto user=google_oauth_user_t::restore(path,email,oauth_purpose_t::cloud);
                            cloud_result_t result{cloud_result_t::kind_t::projects,user};
                            result.projects=google_cloud_projects(user,control);
                            return result;
                        });
                    }},
                    {"移除",[&,email=user.address]
                    {
                        begin_cloud("移除 Google 使用者",[path=settings.account_path,email]
                        { google_oauth_user_t::remove(path,email,oauth_purpose_t::cloud); return cloud_result_t{cloud_result_t::kind_t::remove}; });
                    }}}});
        }
        else if (menu_page==menu_page_t::projects)
            for (const auto& project : projects)
                rows.push_back({project.name,{},false,{{"使用",[&,id=project.id] { select_project(id); }}}});
        else
            for (const auto& account : services)
                rows.push_back({account.address.substr(0,account.address.find('@')),{},false,{{"使用",[&,address=account.address]
                {
                    auto& space=space_by_id(editing_space);
                    if (std::ranges::find(space.accounts,address)==space.accounts.end()) space.accounts.push_back(address);
                    sync_settings();
                    menu_page=menu_page_t::links;
                    if (space.signed_in)
                    {
                        auto candidate=settings;
                        start_settings(candidate,"更新空間服務帳戶",[&,candidate,space]
                        { return runtime.open_space(candidate,space,settings_control); });
                    }
                    else show_menu();
                }}}});
        return rows;
    };
    persistent.m_space_menu_actions = [&]
    {
        using action_t=std::pair<std::string,std::function<void()>>;
        std::vector<action_t> actions;
        if (menu_page==menu_page_t::spaces)
            actions.emplace_back("新增空間",[&]
            {
                name_dialog("新增空間","空間名稱",[&](std::string name)
                {
                    std::random_device random;
                    const auto id=storage_digest(name+std::to_string(random())+
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
                    settings.spaces.push_back({id,std::move(name),{},false});
                    sync_settings();
                    persistent.m_deferred.push_back(show_menu);
                });
            });
        else
        {
            actions.emplace_back("返回",[&]
            {
                menu_page=menu_page==menu_page_t::links ? menu_page_t::spaces :
                    menu_page==menu_page_t::users ? menu_page_t::links :
                    menu_page==menu_page_t::projects ? menu_page_t::users : menu_page_t::projects;
                show_menu();
            });
            if (menu_page==menu_page_t::links)
                actions.emplace_back("新增帳戶",[&] { menu_page=menu_page_t::users; show_menu(); });
            else if (menu_page==menu_page_t::users)
                actions.emplace_back("新增使用者",[&]
                {
                    begin_cloud("新增 Google 使用者",[path=settings.account_path,control=settings_control]
                    {
                        auto user=google_oauth_user_t::login(path,control,oauth_purpose_t::cloud);
                        user->download_picture(control);
                        return cloud_result_t{cloud_result_t::kind_t::login,user};
                    });
                });
            else if (menu_page==menu_page_t::projects)
                actions.emplace_back("新增專案",[&]
                {
                    name_dialog("新增專案","專案名稱",[&](std::string name)
                    {
                        begin_cloud("新增專案："+name,[user=cloud_user,name,control=settings_control]
                        { return cloud_result_t{cloud_result_t::kind_t::created_project,user,google_cloud_create_project(user,control,name)}; });
                    });
                });
            else
                actions.emplace_back("新增帳戶",[&]
                {
                    name_dialog("新增服務帳戶","服務帳戶名稱",[&](std::string name)
                    {
                        google_cloud_validate_account(name);
                        begin_cloud("新增服務帳戶："+name,[user=cloud_user,project=selected_project,name,control=settings_control]
                        {
                            google_cloud_create_account(user,control,project,name);
                            return cloud_result_t{cloud_result_t::kind_t::created_account,user,project};
                        });
                    });
                });
        }
        return actions;
    };
    // Opening the toolbar always starts at the space list; nested pages retain
    // their selection only while navigating within this dropdown workflow.
    persistent.m_toggle_accounts = [&]
    {
        if (persistent.m_account_menu) persistent.m_close_accounts();
        else { menu_page=menu_page_t::spaces; show_menu(); }
    };
    auto update_account_status = [&] { persistent.m_account_status(bool(load_state.m_session)); };
    update_account_status();
    if (!startup_error.empty()) persistent.m_directory_status->set_text("空間登入失敗："+startup_error);
    persistent.m_clear_account = [&]
    {
        if (!load_state.m_session || clear_job.valid() || settings_job.valid())
            return;
        if (clear_retry) clear_retry->hide();
        if (clear_separator) clear_separator->hide();
        if (clear_confirm && clear_retry)
            clear_confirm->set_pos(clear_retry->get_pos());
        clear_control->cancelled = false;
        clear_control->retry();
        clearing_session = load_state.m_session;
        for (const auto& active : runtime.signed_in_sessions())
            if (std::ranges::any_of(active->accounts,[&](const auto& entry)
                { return clearing_session->accounts.contains(entry.first); })) active->clearing=true;
        persistent.m_reset_view();
        clear_progress = 0;
        clear_total = 0;
        clear_control->retry_limit = load_state.m_settings.network_retries;
        for (auto& listing : persistent.m_listings)
            listing.control->cancel();
        std::vector<std::shared_ptr<load_task_t>> cancelled;
        auto account_tasks{load_state.m_task_list};
        account_tasks.insert(account_tasks.end(), load_state.m_finished_tasks.begin(), load_state.m_finished_tasks.end());
        for (auto& task : account_tasks)
            if (task->m_session && std::ranges::any_of(task->m_session->accounts,[&](const auto& entry)
                { return clearing_session->accounts.contains(entry.first); }))
            {
                task->m_control->cancel();
                cancelled.push_back(task);
            }
        multi.interrupt();
        const auto row_id{progress_column->add_object<lgo::divy_flex_soft_t>(
            storage_content_depth, progress_column->space_x(lgo::std_margin), lgo::std_margin,
            lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::top})};
        auto row{progress_column->get_object_shared<lgo::divy_flex_soft_t>(row_id)};
        clear_dismissed = std::make_shared<bool>(false);
        clear_rows.emplace_back(row_id, clear_dismissed);
        clear_text = row->add_object_shared<lgo::text_line_scroll_std_t>(
            storage_content_depth, "清除：列出關聯帳戶全部 Drive 檔案", lgo::std_red_light, lgo::size_1D{256}, false);
        auto clear_actions{row->add_object_shared<lgo::divx_soft_t>(
            storage_content_depth, lgo::div_create_from_member{},
            lgo::size_1D{lgo::font_preference_t::default_get()[0]->leading_get()}, lgo::std_margin,
            row->space_x(lgo::std_margin),
            lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::top})};
        clear_retry = clear_actions->add_object_shared<progress_button_t>(
            storage_content_depth,
            [&]
            {
                runtime.retry_accounts();
                if (clear_job.valid()) clear_control->retry();
                else clear_restart = true;
            },
            "重試", lgo::std_white, lgo::size_1D{32});
        clear_retry->hide();
        clear_separator = clear_actions->add_object_shared<lgo::std_liney_t>(
            storage_content_depth, lgo::size_1D{lgo::font_preference_t::default_get()[0]->leading_get()}, lgo::std_white);
        clear_separator->hide();
        clear_cancel = clear_actions->add_object_shared<progress_button_t>(
            storage_content_depth, [&] { clear_control->cancel(); }, "取消", lgo::std_white, lgo::size_1D{32});
        clear_confirm = clear_actions->add_object_ghost_shared<progress_button_t>(
            storage_content_depth, [dismissed = clear_dismissed] { *dismissed = true; },
            "確定", lgo::std_white, lgo::size_1D{32});
        clear_confirm->hide();
        // Hidden ordinary children still reserve their layout slots. Running
        // cancel and terminal confirm share the first slot with retry; network
        // failure moves cancel to the second slot beside the separator.
        clear_cancel->set_pos(clear_retry->get_pos());
        clear_confirm->set_pos(clear_retry->get_pos());
        clear_job = std::async(std::launch::async,
                               [&, target = clearing_session, cancelled]
                               {
                                   // Do not delete while an old upload/publication can still mutate Drive.
                                   for (auto& task : cancelled)
                                   {
                                       if (auto upload = std::dynamic_pointer_cast<upload_task_t>(task))
                                       {
                                           if (upload->m_prepared.valid())
                                               upload->m_prepared.wait();
                                           if (upload->m_allocation.valid()) upload->m_allocation.wait();
                                           if (upload->m_linked.valid())
                                               upload->m_linked.wait();
                                       }
                                       if (auto download = std::dynamic_pointer_cast<download_task_t>(task))
                                           if (download->m_prepared.valid())
                                               download->m_prepared.wait();
                                       if (auto operation = std::dynamic_pointer_cast<directory_task_t>(task))
                                           if (operation->m_operation.valid()) operation->m_operation.wait();
                                       while (multi.pending(task->m_control))
                                       {
                                           if (clear_control->cancelled)
                                               throw std::runtime_error{"Clear cancelled."};
                                           std::this_thread::sleep_for(std::chrono::milliseconds{5});
                                       }
                                   }
                                   return runtime.clear(target, clear_control, clear_progress, clear_total);
                               });
    };
    auto restore_available_tasks = [&]
    {
        restore_tasks(&load_state,[&](const nlohmann::json& intent) -> std::shared_ptr<storage_session_t>
        {
            for (const auto& active : runtime.signed_in_sessions())
                if (active->storage_id==intent.at("account_namespace").get<std::string>()) return active;
            return {};
        });
    };
    restore_available_tasks();
    if (session && std::filesystem::exists("clear."+storage_digest(session->storage_id)))
        persistent.m_clear_account();
    while (!message_loop(&engine, &persistent, &multi, &load_state, load_state.m_session ? load_state.m_session->queue.get() : nullptr))
    {
        using namespace std::chrono_literals;
        if (cloud_job.valid() && cloud_job.wait_for(0s)==std::future_status::ready)
        {
            persistent.m_switching = false;
            settings_retry->hide();
            settings_cancel->hide();
            settings_separator->hide();
            settings_confirm->set_pos(settings_retry->get_pos());
            settings_confirm->show();
            try
            {
                auto result = cloud_job.get();
                settings_text->set_text("已完成："+settings_task_name,false);
                persistent.m_directory_status->set_text("");
                if (result.user) cloud_user=result.user;
                if (result.kind==cloud_result_t::kind_t::projects)
                { projects=std::move(result.projects); menu_page=menu_page_t::projects; show_menu(); }
                else if (result.kind==cloud_result_t::kind_t::accounts)
                { services=std::move(result.accounts); menu_page=menu_page_t::services; show_menu(); }
                else if (result.kind==cloud_result_t::kind_t::created_project)
                {
                    begin_cloud("讀取專案",[user=cloud_user,control=settings_control]
                    {
                        cloud_result_t next{cloud_result_t::kind_t::projects,user};
                        next.projects=google_cloud_projects(user,control);
                        return next;
                    });
                }
                else if (result.kind==cloud_result_t::kind_t::created_account)
                    select_project(result.project);
                else
                { menu_page=menu_page_t::users; glfwFocusWindow(engine.window_get()); show_menu(); }
            }
            catch (const std::exception& error)
            {
                settings_text->set_text(settings_control->cancelled ? "已取消："+settings_task_name :
                    "失敗："+settings_task_name+"（"+error.what()+"）",false);
                persistent.m_directory_status->set_text(settings_control->cancelled ? "操作已取消" : "帳戶操作失敗");
                show_menu();
            }
        }
        if (settings_job.valid() && settings_job.wait_for(0s) == std::future_status::ready)
        {
            try
            {
                auto result{settings_job.get()};
                auto next_font{
                    std::make_unique<lgo::font_face_t>(result.settings.font_path, lgo::std_font_size.x)};
                result.settings.save();
                lgo::init_logfile(log_path(result.settings));
                http_error_log_codes_set(storage_settings_t::parse_http_log_codes(result.settings.http_log_codes));
                const bool changed_location{!result.session || !load_state.m_session ||
                    result.session->storage_id != load_state.m_session->storage_id ||
                    result.settings.root_path != settings.root_path ||
                    result.session->root != load_state.m_session->root};
                if (changed_location)
                {
                    persistent.m_reset_view();
                    pos = result.session ? result.session->root : "";
                }
                settings = result.settings;
                load_state.m_settings = settings;
                load_state.m_session = result.session;
                google_account_t::default_shared_set(result.session ? result.session->account : nullptr);
                update_account_status();
                restore_after_login = bool(result.session);
                fonts.push_back(std::move(next_font));
                lgo::font_preference_t::default_get().assign(1, fonts.back().get());
                persistent.set_directory_header_font(settings.font_path);
                lgo::text_line_t::reload_fonts();
                settings_text->set_text("已完成："+settings_task_name,false);
                persistent.m_refresh();
                // Only operations started from the space menu return to it.
                // Closing the independent settings dialog leaves it closed.
                persistent.m_deferred.push_back(show_menu);
            }
            catch (const std::exception& error)
            {
                persistent.m_directory_status->set_text("空間或設定操作失敗");
                settings_text->set_text(settings_control->cancelled ?
                    "已取消："+settings_task_name : "失敗："+settings_task_name+"（"+error.what()+"）",false);
                lgo::log_file(std::string{"Space/settings operation failed: "} + error.what() + "\n");
            }
            settings_retry->hide();
            settings_cancel->hide();
            settings_separator->hide();
            settings_confirm->set_pos(settings_retry->get_pos());
            settings_confirm->show();
            persistent.m_switching = false;
        }
        if (restore_after_login && !std::ranges::any_of(load_state.m_finished_tasks,
            [](const auto& task){ return task->m_result==task_result_t::signed_out; }))
        {
            restore_available_tasks();
            restore_after_login = false;
        }
        if (settings_job.valid())
        {
            if (settings_control->paused)
            {
                persistent.m_directory_status->set_text("設定更新失敗");
                settings_text->set_text("網路失敗："+settings_task_name+"（"+settings_control->reason()+"）",false);
                settings_cancel->set_pos(settings_separator->get_pos()+
                    lgo::size_2D{settings_separator->get_size().x+lgo::std_margin.x,0});
                settings_retry->show();
                settings_separator->show();
            }
            else
            {
                settings_text->set_text(settings_task_name+"（處理中）",false);
                settings_retry->hide();
                settings_separator->hide();
                settings_cancel->set_pos(settings_retry->get_pos());
            }
        }
        std::erase_if(settings_rows,[&](const auto& row)
        {
            if (!*row.second) return false;
            progress_column->remove_object(row.first);
            return true;
        });
        if (clear_restart)
        {
            clear_restart = false;
            *clear_dismissed = true;
            persistent.m_clear_account();
        }
        std::erase_if(clear_rows, [&](const auto& row)
        {
            if (!*row.second) return false;
            progress_column->remove_object(row.first);
            return true;
        });
        if (clear_job.valid())
        {
            const auto second_slot{clear_separator->get_pos() +
                lgo::size_2D{clear_separator->get_size().x + lgo::std_margin.x, 0}};
            if (clear_control->cancelled)
            {
                clear_text->set_text("清除：取消中", false);
                clear_retry->hide();
                clear_cancel->hide();
                clear_separator->hide();
            }
            else if (clear_control->paused)
            {
                clear_text->set_text("清除失敗：" + clear_control->reason(), false);
                clear_retry->show();
                clear_separator->show();
                clear_cancel->set_pos(second_slot);
            }
            else
            {
                clear_retry->hide();
                clear_separator->hide();
                clear_cancel->set_pos(clear_retry->get_pos());
                clear_text->set_text("清除：" + std::to_string(clear_progress.load()) + "/" +
                                         std::to_string(clear_total.load()), false);
            }
            if (clear_job.wait_for(0s) == std::future_status::ready)
            {
                bool failed{};
                try
                {
                    auto root{clear_job.get()};
                    runtime.cleared(clearing_session, root);
                    pos = root;
                    clear_text->set_text("清除完成", false);
                    persistent.m_refresh();
                }
                catch (const std::exception& error)
                {
                    failed = !clear_control->cancelled;
                    clear_text->set_text(clear_control->cancelled ? "清除：已取消" :
                        std::string{"清除未完成："} + error.what(), false);
                }
                clear_cancel->hide();
                clear_retry->hide();
                clear_separator->hide();
                clear_confirm->set_pos(clear_retry->get_pos());
                if (failed)
                {
                    clear_retry->show();
                    clear_separator->show();
                    clear_confirm->set_pos(second_slot);
                }
                clear_confirm->show();
                clearing_session.reset();
            }
        }
        lgo::text_line_t::pending_load();
    }
    // Space membership and sign-in state are durable independently of tasks.
    settings.save();
    engine.flush();
    state.clear_state();
    if (!engine.gpu_failure_reason().empty())
        throw lgo::vulkan_unsupported_t{std::string{engine.gpu_failure_reason()}};
}

// Report fatal graphics failures after stack unwinding has stopped transfers,
// saved the unused blob queue and destroyed the window/GPU resources.
auto report_error(std::string_view title, const std::exception& error) noexcept -> void
{
    try
    {
        const std::string message{std::string{title} + ": " + error.what() +
                                  "\nSee the configured logfile directory for details."};
        lgo::log_file(message + "\n");
        MessageBoxW(nullptr, lgo::to_wstring(message).c_str(), L"Storage", MB_OK | MB_ICONERROR);
    }
    catch (...)
    {
    }
}

auto __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) -> int
{
    int result{};
    try
    {
        app();
    }
    catch (const vk::SystemError& error)
    {
        report_error("Vulkan error", error);
        result = 1;
    }
    catch (const lgo::vulkan_unsupported_t& error)
    {
        report_error("Unsupported Vulkan configuration or unrecoverable GPU failure", error);
        result = 1;
    }
    catch (const std::exception& error)
    {
        report_error("Application error", error);
        result = 1;
    }
    catch (...)
    {
        try
        {
            lgo::log_file("uncaught exception: non-std exception reached wWinMain.\n");
        }
        catch (...)
        {
        }
        MessageBoxW(nullptr, L"Unknown application error. See the configured logfile directory.", L"Storage",
                    MB_OK | MB_ICONERROR);
        result = 1;
    }
    lgo::log_file("finished_all!\n");
    return result;
}
