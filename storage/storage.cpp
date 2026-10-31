#include <nlohmann/json.hpp>

import std;

import external.Windows;
import external.Vulkan;
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
    auto log_path = [](const storage_settings_t& value)
    {
        return (std::filesystem::path{value.logfile_path} / (std::to_string(std::time(nullptr)) + ".txt"))
            .string();
    };
    lgo::init_logfile(log_path(settings));
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
    const auto login_record = [](const std::shared_ptr<storage_session_t>& account)
    {
        return storage_account_login_t{account->address,account->account_path,
            std::filesystem::path{account->root_path}.parent_path().generic_string(),bool(account->oauth)};
    };
    const auto remember_session = [&](storage_settings_t& value, const std::shared_ptr<storage_session_t>& account)
    {
        if (!value.signed_in_accounts) value.signed_in_accounts.emplace();
        if (account) (*value.signed_in_accounts)[account->storage_id] = login_record(account);
        value.selected_account = account ? account->storage_id : std::string{};
    };
    auto startup_control{std::make_shared<request_control_t>()};
    std::shared_ptr<storage_session_t> session;
    std::string startup_error;
    if (!std::filesystem::exists(settings.account_path) && settings.account_path == "key.json" &&
        std::filesystem::exists("client.json")) settings.account_path = "client.json";
    if (settings.signed_in_accounts)
    {
        // Restore every explicitly signed-in identity, including accounts with
        // no transfer journal. A failure for one must not hide the others.
        for (const auto& [id,account] : *settings.signed_in_accounts)
            try
            {
                auto saved = settings;
                saved.account_path = account.account_path;
                saved.root_path = account.root_directory;
                auto restored = runtime.open(saved,startup_control,account.address,{},id);
                if (!restored || restored->storage_id!=id || bool(restored->oauth)!=account.oauth)
                    throw std::runtime_error{"Saved login does not match its credential file."};
                if (id==settings.selected_account) session = std::move(restored);
            }
            catch (const std::exception& error) { startup_error = error.what(); }
    }
    else
    {
        try
        {
            if (std::filesystem::exists(settings.account_path)) session = runtime.open(settings,startup_control);
        }
        catch (const std::exception& error) { startup_error = error.what(); }
    }
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
    std::string selected_email, settings_task_name;
    std::vector<std::pair<std::uint64_t,std::shared_ptr<bool>>> settings_rows;
    bool account_action{}, restore_after_login{};
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
    auto apply_settings = [&](storage_settings_t candidate, bool interactive)
    {
        if (settings_job.valid())
            throw std::runtime_error{"設定正在套用，請稍候"};
        if (clearing_session || (load_state.m_session && load_state.m_session->clearing))
            throw std::runtime_error{"清除期間請稍後再切換設定"};
        // Validate font and output paths before accepting the form.
        auto next_font{std::make_unique<lgo::font_face_t>(candidate.font_path, lgo::std_font_size.x)};
        std::filesystem::create_directories(candidate.logfile_path);
        const auto probe_path{std::filesystem::path{candidate.logfile_path} /
                              (".storage-write-test." +
                               std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
        std::ofstream probe{probe_path, std::ios::binary};
        if (!probe)
            throw std::runtime_error{"無法寫入日誌資料夾"};
        probe.close();
        std::filesystem::remove(probe_path);
        settings_control->retry();
        settings_control->cancelled = false;
        settings_control->retry_limit = candidate.network_retries;
        const auto email = std::exchange(selected_email,{});
        begin_settings_row(interactive ? "新增帳戶" : email.empty() ? "更新設定" : "切換帳戶："+email,true);
        persistent.m_switching = true;
        persistent.m_directory_status->set_text(interactive ? "登入中" : "目錄更新中");
        settings_job =
            std::async(std::launch::async, [&, candidate, interactive, email]
            {
                auto target = interactive ? runtime.open_oauth(candidate,
                    google_oauth_user_t::login(candidate.account_path,settings_control),settings_control) :
                    runtime.open(candidate,settings_control,email);
                if (target && target->oauth && !email.empty())
                    google_oauth_user_t::select(candidate.account_path,email);
                return settings_result_t{candidate,std::move(target)};
            });
    };
    persistent.m_apply_settings = [&](storage_settings_t candidate) { apply_settings(std::move(candidate),false); };
    auto oauth_settings = [&]
    {
        auto candidate = settings;
        candidate.account_path = "client.json";
        if (std::filesystem::exists(settings.account_path))
        {
            const auto config = nlohmann::json::parse(storage_read(settings.account_path));
            if (config.contains("installed")) candidate.account_path = settings.account_path;
        }
        return candidate;
    };
    persistent.m_service_accounts = [] { return storage_service_accounts(); };
    persistent.m_select_service_account = [&](const service_account_file_t& account)
    {
        try
        {
            selected_email = account.address;
            auto candidate = settings;
            candidate.account_path = account.account_path;
            candidate.signed_out_service_accounts.erase(account.address);
            apply_settings(std::move(candidate),false);
        }
        catch (const std::exception&) { selected_email.clear(); persistent.m_directory_status->set_text("無法登入服務帳戶"); }
    };
    persistent.m_saved_accounts = [&]
    {
        const auto candidate = oauth_settings();
        if (!std::filesystem::exists(candidate.account_path)) return std::vector<oauth_saved_account_t>{};
        return google_oauth_user_t::saved_accounts(candidate.account_path);
    };
    persistent.m_login = [&]
    {
        try { apply_settings(oauth_settings(),true); }
        catch (const std::exception&) { persistent.m_directory_status->set_text("無法開始登入"); }
    };
    persistent.m_select_account = [&](const std::string& email)
    {
        try
        {
            selected_email = email;
            apply_settings(oauth_settings(),false);
        }
        catch (const std::exception&) { selected_email.clear(); persistent.m_directory_status->set_text("無法登入帳號"); }
    };
    auto menu_account_id = [&](const std::string& email, bool oauth)
    {
        if (!oauth) return email;
        const auto config = nlohmann::json::parse(storage_read(oauth_settings().account_path));
        return "appdata:"+storage_digest(config.at("installed").at("client_id").get<std::string>())+":"+email;
    };
    persistent.m_account_signed_in = [&](const std::string& email, bool oauth)
    {
        try
        {
            const auto id = menu_account_id(email,oauth);
            return settings.signed_in_accounts && settings.signed_in_accounts->contains(id);
        }
        catch (const std::exception&) { return false; }
    };
    persistent.m_has_signed_in_accounts = [&]
        { return settings.signed_in_accounts && !settings.signed_in_accounts->empty(); };
    auto suspend_account_tasks = [&](const std::string& id)
    {
        // Keep every checkpoint; this is shutdown, not user cancellation.
        auto tasks = load_state.m_task_list;
        for (auto& task : tasks)
            if (task->m_session && task->m_session->storage_id==id)
                finish_task(*task,task_result_t::signed_out,"等待帳號登入："+task->m_label);
        multi.interrupt();
    };
    persistent.m_remove_account = [&](const std::string& email)
    {
        if (persistent.m_switching || clearing_session || persistent.m_account_signed_in(email,true)) return;
        try
        {
            const auto candidate = oauth_settings();
            const auto user = google_oauth_user_t::restore(candidate.account_path,email);
            suspend_account_tasks(user->storage_id());
            begin_settings_row("移除帳戶："+email,false);
            persistent.m_switching = account_action = true;
            persistent.m_directory_status->set_text("移除帳號中");
            settings_job = std::async(std::launch::async,[&,candidate,email,id=user->storage_id(),current=load_state.m_session]
            {
                runtime.sign_out(id);
                google_oauth_user_t::remove(candidate.account_path,email);
                return settings_result_t{settings,current};
            });
        }
        catch (const std::exception&) { persistent.m_directory_status->set_text("無法移除帳號"); }
    };
    auto logout_accounts = [&](storage_account_logins_t targets, bool all)
    {
        if (targets.empty() || settings_job.valid() || clearing_session) return;
        auto candidate = settings;
        std::string oauth_path;
        if (all)
        {
            // Include known credentials even if their account currently has no
            // task/session, so journal restoration cannot log it back in.
            for (const auto& account : storage_service_accounts())
                candidate.signed_out_service_accounts.insert(account.address);
            try
            {
                const auto path = oauth_settings().account_path;
                if (std::filesystem::exists(path) &&
                    nlohmann::json::parse(storage_read(path)).contains("installed")) oauth_path = path;
            }
            catch (const std::exception&) { } // Live sessions still carry their own client paths.
        }
        auto current = load_state.m_session;
        const auto selected = [&](const std::shared_ptr<storage_session_t>& session)
        {
            return session && targets.contains(session->storage_id);
        };
        for (const auto& [id,account] : targets) suspend_account_tasks(id);
        for (auto& listing : persistent.m_listings)
            if (selected(listing.session)) listing.control->cancel();
        if (persistent.m_clipboard && selected(persistent.m_clipboard->session)) persistent.m_clipboard.reset();
        if (selected(current))
        {
            persistent.m_reset_view();
            current.reset();
        }
        begin_settings_row(all ? "登出所有" : "登出："+targets.begin()->second.address,false);
        persistent.m_switching = account_action = true;
        persistent.m_directory_status->set_text("登出中");
        settings_job = std::async(std::launch::async,
            [&,targets=std::move(targets),candidate=std::move(candidate),current,all,oauth_path]() mutable
        {
            for (const auto& [id,account] : targets)
            {
                runtime.sign_out(id);
                if (account.oauth)
                    google_oauth_user_t::sign_out(account.account_path,all ? std::string{} : account.address);
                else candidate.signed_out_service_accounts.insert(account.address);
                if (candidate.signed_in_accounts) candidate.signed_in_accounts->erase(id);
            }
            if (!oauth_path.empty()) google_oauth_user_t::sign_out(oauth_path);
            return settings_result_t{std::move(candidate),current};
        });
    };
    persistent.m_logout_account = [&](const std::string& email, bool oauth)
    {
        try
        {
            const auto id = menu_account_id(email,oauth);
            if (settings.signed_in_accounts)
                if (const auto account = settings.signed_in_accounts->find(id); account!=settings.signed_in_accounts->end())
                    logout_accounts({*account},false);
        }
        catch (const std::exception&) { persistent.m_directory_status->set_text("無法登出帳號"); }
    };
    persistent.m_logout = [&]
    {
        try
        {
            if (settings.signed_in_accounts) logout_accounts(*settings.signed_in_accounts,true);
        }
        catch (const std::exception&) { persistent.m_directory_status->set_text("無法登出所有帳號"); }
    };
    auto update_account_status = [&]
    {
        persistent.m_account_status(bool(load_state.m_session));
    };
    update_account_status();
    if (!startup_error.empty())
    {
        persistent.m_directory_status->set_text("請重新登入");
        lgo::log_file("Startup account unavailable; sign in or update account settings.\n");
    }
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
        clearing_session->clearing = true;
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
            if (task->m_session && task->m_session->storage_id == clearing_session->storage_id)
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
            storage_content_depth, clearing_session->oauth ? "清除：列出應用程式資料" : "清除：列出全部 Drive 檔案", lgo::std_red_light, lgo::size_1D{256}, false);
        auto clear_actions{row->add_object_shared<lgo::divx_soft_t>(
            storage_content_depth, lgo::div_create_from_member{},
            lgo::size_1D{lgo::font_preference_t::default_get()[0]->leading_get()}, lgo::std_margin,
            row->space_x(lgo::std_margin),
            lgo::alignment_2D{lgo::alignment_x::left, lgo::alignment_y::top})};
        clear_retry = clear_actions->add_object_shared<progress_button_t>(
            storage_content_depth,
            [&]
            {
                load_state.m_session->account->retry();
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
        restore_tasks(&load_state,
            [&](const nlohmann::json& intent) -> std::shared_ptr<storage_session_t>
            {
                const auto space = intent.value("account_namespace", "");
                if (settings.signed_in_accounts)
                {
                    auto login = settings.signed_in_accounts->find(space);
                    if (space.empty())
                    {
                        const auto email = intent.at("account_email").get<std::string>();
                        const auto path = intent.at("account_path").get<std::string>();
                        login = std::ranges::find_if(*settings.signed_in_accounts,[&](const auto& entry)
                            { return entry.second.address==email && entry.second.account_path==path; });
                    }
                    if (login==settings.signed_in_accounts->end()) return {};
                    // Use the matching restored login, rather than requiring it
                    // to be the account currently selected in the browser.
                    for (const auto& active : runtime.signed_in_sessions())
                        if (active->storage_id==login->first && !active->root.empty())
                        {
                            auto saved = settings;
                            saved.account_path = login->second.account_path;
                            saved.root_path = intent.value("root_directory",login->second.root_directory);
                            if (active->oauth)
                                return runtime.open_oauth(saved,active->oauth,startup_control,intent.at("root"));
                            return runtime.open(saved,startup_control,active->address,intent.at("root"),login->first);
                        }
                    return {};
                }
                if (space.starts_with("appdata:"))
                {
                    if (!load_state.m_session || load_state.m_session->storage_id!=space)
                        return {}; // Keep signed-out account journals until that user logs in.
                    auto saved = settings;
                    saved.account_path = intent.at("account_path");
                    // Old journals already carry the cloud root ID; after manual
                    // relocation they use the configured root directory.
                    saved.root_path = intent.value("root_directory",settings.root_path);
                    // The task's root can differ from the user's current browser root.
                    return runtime.open_oauth(saved,load_state.m_session->oauth,startup_control,intent.at("root"));
                }
                auto saved = settings;
                saved.account_path = intent.at("account_path");
                saved.root_path = intent.value("root_directory",settings.root_path);
                try { return runtime.open(saved,startup_control,intent.at("account_email"),intent.at("root"),space); }
                catch (const std::exception&) { return {}; }
            });
    };
    restore_available_tasks();
    if (!settings.signed_in_accounts)
    {
        // Migrate only identities actually recovered from legacy selection/tasks;
        // saved credential history alone does not mean that a user is signed in.
        remember_session(settings,session);
        for (const auto& task : load_state.m_task_list)
            if (task->m_session) (*settings.signed_in_accounts)[task->m_session->storage_id] = login_record(task->m_session);
        load_state.m_settings = settings;
    }
    if (session && session->clearing)
        persistent.m_clear_account();
    while (!message_loop(&engine, &persistent, &multi, &load_state, load_state.m_session ? load_state.m_session->queue.get() : nullptr))
    {
        using namespace std::chrono_literals;
        if (settings_job.valid() && settings_job.wait_for(0s) == std::future_status::ready)
        {
            try
            {
                auto result{settings_job.get()};
                auto next_font{
                    std::make_unique<lgo::font_face_t>(result.settings.font_path, lgo::std_font_size.x)};
                remember_session(result.settings,result.session);
                result.settings.save();
                lgo::init_logfile(log_path(result.settings));
                const bool changed_location{!result.session || !load_state.m_session ||
                    result.session->account != load_state.m_session->account ||
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
                if (settings_task_name=="新增帳戶" && result.session)
                    settings_task_name += "："+result.session->address;
                settings_text->set_text("已完成："+settings_task_name,false);
                persistent.m_refresh();
            }
            catch (const std::exception& error)
            {
                persistent.m_directory_status->set_text(account_action ? "帳號操作失敗" : "設定套用失敗");
                settings_text->set_text(!account_action && settings_control->cancelled ?
                    "已取消："+settings_task_name : "失敗："+settings_task_name+"（"+error.what()+"）",false);
                lgo::log_file(std::string{account_action ? "Account action failed: " : "Settings failed: "} + error.what() + "\n");
            }
            settings_retry->hide();
            settings_cancel->hide();
            settings_separator->hide();
            settings_confirm->set_pos(settings_retry->get_pos());
            settings_confirm->show();
            persistent.m_switching = false;
            account_action = false;
        }
        if (restore_after_login && !std::ranges::any_of(load_state.m_finished_tasks,
            [](const auto& task){ return task->m_result==task_result_t::signed_out; }))
        {
            restore_available_tasks();
            restore_after_login = false;
        }
        if (settings_job.valid() && !account_action)
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
    // Account actions save immediately; also persist a legacy login snapshot
    // when a session was simply opened and closed without any account action.
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
