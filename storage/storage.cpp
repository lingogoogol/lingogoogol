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
    auto startup_control{std::make_shared<request_control_t>()};
    auto session{runtime.open(settings, startup_control)};
    google_account_t::default_shared_set(session->account);
    load_state_t load_state;
    load_state.m_session = session;
    load_state.m_settings = settings;
    curl_multi_worker_t multi;
    std::string pos{session->root};
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
    std::shared_ptr<lgo::button_classic_std_t> settings_retry, settings_cancel;
    struct shutdown_t
    {
        storage_runtime_t& runtime;
        load_state_t& tasks;
        curl_multi_worker_t& multi;
        GUI_persistent_t& gui;
        std::shared_ptr<request_control_t> settings;
        ~shutdown_t()
        {
            close_app = true;
            settings->cancel();
            for (auto& listing : gui.m_listings)
                listing.control->cancel();
            // Shutdown cancellation is deliberately not saved as user cancellation.
            // Every task's last durable checkpoint stays available for next launch.
            for (auto& task : tasks.m_task_list)
                task->m_control->cancel();
            runtime.stop();
            multi.stop();
        }
    } shutdown{runtime, load_state, multi, persistent, settings_control};
    home(&engine, &state, &persistent, &progress_column, &load_state, &multi, session->queue.get(), &pos);
    persistent.m_apply_settings = [&](storage_settings_t candidate)
    {
        if (settings_job.valid())
            throw std::runtime_error{"設定正在套用，請稍候"};
        if (load_state.m_session->clearing)
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
        settings_text = progress_column->add_object_shared<lgo::text_line_scroll_std_t>(
            lgo::depth_range_t{0, .5f}, "設定更新中", lgo::std_red, lgo::size_1D{256}, false);
        settings_retry = progress_column->add_object_shared<lgo::button_classic_std_t>(
            lgo::depth_range_t{0, .5f},
            [&]
            {
                runtime.retry_accounts();
                settings_control->retry();
            },
            "重試");
        settings_retry->hide();
        settings_cancel = progress_column->add_object_shared<lgo::button_classic_std_t>(
            lgo::depth_range_t{0, .5f}, [&] { settings_control->cancel(); }, "取消");
        persistent.m_switching = true;
        persistent.m_directory_status->set_text("目錄更新中");
        settings_job =
            std::async(std::launch::async, [&, candidate]
                       { return settings_result_t{candidate, runtime.open(candidate, settings_control)}; });
    };
    restore_tasks(&load_state,
                  [&](const nlohmann::json& intent)
                  {
                      auto saved{settings};
                      saved.account_path = intent.at("account_path");
                      saved.root_path = intent.at("root_path");
                      return runtime.open(saved, startup_control, intent.at("account_email"),
                                          intent.at("root"));
                  });
    while (!message_loop(&engine, &persistent, &multi, &load_state, load_state.m_session->queue.get()))
    {
        using namespace std::chrono_literals;
        if (settings_job.valid() && settings_job.wait_for(0s) == std::future_status::ready)
        {
            try
            {
                auto result{settings_job.get()};
                auto next_font{
                    std::make_unique<lgo::font_face_t>(result.settings.font_path, lgo::std_font_size.x)};
                result.settings.save();
                lgo::init_logfile(log_path(result.settings));
                const bool changed_location{result.session->account != load_state.m_session->account ||
                                            result.settings.root_path != settings.root_path ||
                                            result.session->root != load_state.m_session->root};
                if (changed_location)
                {
                    persistent.m_reset_view();
                    pos = result.session->root;
                }
                settings = result.settings;
                load_state.m_settings = settings;
                load_state.m_session = result.session;
                google_account_t::default_shared_set(result.session->account);
                fonts.push_back(std::move(next_font));
                lgo::font_preference_t::default_get().assign(1, fonts.back().get());
                lgo::text_line_t::reload_fonts();
                settings_text->set_text("設定已套用", false);
                persistent.m_refresh();
            }
            catch (const std::exception& error)
            {
                persistent.m_directory_status->set_text("設定套用失敗");
                settings_text->set_text(std::string{"設定失敗："} + error.what(), false);
                lgo::log_file(std::string{"Settings failed: "} + error.what() + "\n");
            }
            settings_retry->hide();
            settings_cancel->hide();
            persistent.m_switching = false;
        }
        if (settings_job.valid())
        {
            if (settings_control->paused)
            {
                persistent.m_directory_status->set_text("設定更新失敗");
                settings_text->set_text("設定失敗：" + settings_control->reason(), false);
                settings_retry->show();
            }
            else
                settings_retry->hide();
        }
        lgo::text_line_t::pending_load();
    }
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
