#include <nlohmann/json.hpp>
import std;
import lgo.io;
import lgo.math;
import lgo.GUI;
import external.GLFW;
import storage.http;
import storage.google;
import storage.kernel;
using namespace std::chrono_literals;

// Phone-width layout (storage_compact_width): run_compact_layout.py captures each
// requested screen through capture-request and checks the window geometry.
int main()
{
    try
    {
        auto check = [](bool ok, const char* text) { if (!ok) throw std::runtime_error{text}; };
        lgo::init_logfile("logfile/compact-test.txt");
        lgo::FreeType_t freetype;
        lgo::font_face_t::freetype_set(&freetype);
        {
            // At a phone display scale (2.75 framebuffer pixels per logical pixel),
            // each glyph bitmap must fill its whole-logical-pixel quad at its own
            // resolution: padded, never stretched, so glyph sizes stay consistent.
            constexpr double phone_scale{2.75};
            lgo::font_face_t::raster_scale_set(static_cast<float>(phone_scale));
            lgo::font_face_t phone{"font/NotoSansTC-VariableFont_wght.ttf", 16};
            lgo::font_face_t::raster_scale_set(1);
            lgo::font_preference_t preference;
            preference.assign(1, &phone);
            for (char32_t code : std::u32string_view{U"Google API 已完成：讀取使用者 frame_rate: 44"})
            {
                const lgo::font_char_t glyph{code, &preference};
                const auto quad{glyph.bbox_size_get()};
                const auto bitmap{glyph.pixmap_get().dim_get()};
                if (!quad.x || !quad.y) continue;
                check(std::abs(static_cast<double>(bitmap.x) - static_cast<double>(quad.x) * phone_scale) <= 1 &&
                      std::abs(static_cast<double>(bitmap.y) - static_cast<double>(quad.y) * phone_scale) <= 1,
                      "A glyph bitmap is stretched to its quad at a phone display scale.");
                const auto shift{glyph.bbox_shift_get()};
                check(std::abs(shift[0]) * phone_scale <= 0.5 + 1e-4 && std::abs(shift[1]) * phone_scale <= 0.5 + 1e-4,
                      "A glyph is placed more than half a framebuffer pixel from its real position.");
            }
        }
        lgo::font_face_t face{"font/NotoSansTC-VariableFont_wght.ttf", 16};
        lgo::font_preference_t::default_get().assign(1, &face);
        lgo::engine_t engine{lgo::size_2D{412, 860}, "Compact layout regression"};
        struct cleanup_t { lgo::engine_t& engine; ~cleanup_t() { engine.flush(); lgo::text_line_t::free(); } } cleanup{engine};
        lgo::depth_tracker_t depth;
        lgo::state_t gui{&engine, &depth};
        std::atomic_bool close{true}; // Never refill reserves.
        auto session = std::make_shared<storage_session_t>();
        session->account = std::make_shared<google_account_t>();
        session->account_path = "unused"; session->address = "test"; session->storage_id = "fixture";
        session->root = "root"; session->root_path = "root/fixture"; session->root_account = "fixture";
        session->accounts["fixture"] = session->account;
        session->queue = std::make_shared<blob_queue_t>("queue", session->account.get(), &close);
        session->queues["fixture"] = session->queue;
        session->remember("root", "fixture", "folder");
        google_account_t::default_shared_set(session->account);
        load_state_t loads; loads.m_session = session; loads.m_settings.network_retries = 0;
        curl_multi_worker_t multi;
        GUI_persistent_t persistent;
        persistent.m_space_menu_rows = []
        {
            return std::vector<GUI_persistent_t::menu_row_t>{
                {"test", "", false, {{"登入", {}}, {"登出", [] {}}, {"編輯", [] {}}, {"移除", [] {}}}},
                {"工作備份", "", false, {{"登入", [] {}}, {"登出", {}}, {"編輯", [] {}}, {"移除", [] {}}}}};
        };
        persistent.m_space_menu_actions = []
        {
            return std::vector<std::pair<std::string, std::function<void()>>>{{"繼續配置", {}}, {"新增空間", [] {}}};
        };
        std::shared_ptr<lgo::divy_soft_t> column;
        std::string pos = "root";
        home(&engine, &gui, &persistent, &column, &loads, &multi, session->queue.get(), &pos);
        check(engine.get_window_size().x < storage_compact_width, "The test window is not compact.");
        auto pump = [&]
        {
            check(!message_loop(&engine, &persistent, &multi, &loads, session->queue.get()), "Window closed");
            lgo::text_line_t::pending_load();
            std::this_thread::sleep_for(1ms);
        };
        auto wait = [&](auto done)
        {
            const auto deadline = std::chrono::steady_clock::now() + 45s;
            while (!done()) { check(std::chrono::steady_clock::now() < deadline, "Timed out"); pump(); }
        };
        auto capture = [&](const std::string& name)
        {
            for (int i = 0; i < 90; ++i) pump(); // Let animations and deferred text settle.
            std::ofstream{"capture-request"} << name;
            wait([] { return !std::filesystem::exists("capture-request"); });
        };
        wait([&] { return persistent.m_listings.empty() && !persistent.m_listing_builder; });
        check(!persistent.m_directory_rows.empty(), "The listing is empty.");
        capture("main");
        persistent.m_open_accounts(false);
        capture("spaces");
        persistent.m_close_accounts();
        folder_create_callback(&engine, &persistent, &loads, &pos);
        capture("dialog");
        persistent.m_dialog.reset();
        file_download_callback(&loads, "IMG_20261103_091522.jpg", "photo");
        wait([&] { return !loads.m_task_list.empty(); });
        capture("task");
        std::cout << "PASS: compact main screen, space menu, dialog and task row captured\n";
    }
    catch (const std::exception& error)
    {
        std::cout << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
