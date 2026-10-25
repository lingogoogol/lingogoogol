#include <nlohmann/json.hpp>

import std;
import lgo.io;
import lgo.math;
import lgo.GUI;
import storage.http;
import storage.google;
import storage.kernel;

using namespace std::chrono_literals;

auto check(bool value, const char* message) -> void
{
    if (!value) throw std::runtime_error{ message };
}

int main()
{
    std::set_terminate([] { std::_Exit(42); });
    try
    {
        lgo::init_logfile("logfile/folder-download.txt");
        lgo::FreeType_t freetype{};
        lgo::font_face_t::freetype_set(&freetype);
        lgo::font_face_t face{ "font/NotoSansTC-VariableFont_wght.ttf", 24 };
        lgo::font_preference_t::default_get().push_back(&face);
        lgo::engine_t engine{ lgo::size_2D{ 1536, 1024 }, "Folder download regression" };
        lgo::depth_tracker_t depth{};
        lgo::state_t gui{ &engine, &depth };
        load_state_t loads{};
        std::atomic_bool close{ true }; // Never replenish synthetic blobs or use live cloud data.
        blob_queue_t queue{ "blob_queue", &google_account_t::default_get(), &close };
        std::string pos{ "root" };
        std::shared_ptr<lgo::divy_soft_t> column{};
        GUI_persistent_t persistent{};
        curl_multi_t multi{};
        home(&engine, &gui, &persistent, &column, &loads, &multi, &queue, &pos);
        check(queue.count() == 21000, "This test requires the upload reserve threshold.");
        auto add = [&](const std::string& id, const std::filesystem::path& path, bool folder = true)
        {
            // The fixture replaces only the native picker with an "output" selection;
            // exercise the production callbacks and their actual async preparation.
            const auto count{ loads.m_task_list.size() };
            if (folder) folder_download_callback(&loads, path.filename().string(), id);
            else file_download_callback(&loads, path.filename().string(), id);
            check(loads.m_task_list.size() == count + 1, "Download callback did not create a task.");
            return std::dynamic_pointer_cast<download_task_t>(loads.m_task_list.back());
        };
        auto nested{ add("folder-main", "output/nested") };
        auto concurrent{ add("folder-main", "output/concurrent") };
        auto empty{ add("folder-empty", "output/empty") };
        auto zeros{ add("folder-zeros", "output/zeros") };
        auto empty_file{ add("slow-empty", "output/single-empty.bin", false) };
        // Browsing uses the GUI thread's connection while preparation uses worker-local connections.
        for (int i{}; i != 3; ++i) check(list_folder_content(google_easy_t::get(), &pos).empty(), "Browsing returned wrong data.");
        const auto deadline{ std::chrono::steady_clock::now() + 20s };
        while (!loads.m_task_list.empty())
        {
            check(std::chrono::steady_clock::now() < deadline, "Folder download stalled.");
            check(!message_loop(&engine, &persistent, &multi, &loads, &queue), "GUI closed during preparation/download.");
            lgo::text_line_t::pending_load();
        }
        check(loads.m_end.empty(), "A completed download still has pending callbacks.");
        for (const auto& directory : { "output/nested", "output/concurrent" })
        {
            const std::filesystem::path path{ directory };
            check(lgo::get_file_str((path / "payload.bin").string()) == "ABCDEF", "Multipart file content differs.");
            check(lgo::get_file_str((path / "sub/child.bin").string()) == "nested", "Nested file content differs.");
            check(lgo::get_file_str((path / "page-two.bin").string()) == "page two", "Paginated folder entry missing.");
            check(std::filesystem::file_size(path / "empty.bin") == 0, "Empty file missing.");
            check(std::filesystem::is_directory(path / "empty-dir"), "Empty subfolder missing.");
        }
        check(std::filesystem::is_directory("output/empty"), "Empty root directory missing.");
        check(std::filesystem::is_directory("output/zeros/child"), "Nested empty directory missing.");
        check(std::filesystem::file_size("output/zeros/zero.bin") == 0, "Zero-only folder file missing.");
        check(std::filesystem::file_size("output/single-empty.bin") == 0, "Standalone empty file missing.");
        check(nested->m_progress == nested->m_size && nested->m_files_remaining == 0, "Task completion counters differ.");
        std::println("PASS: concurrent recursive folders, pagination, empty files/directories, and delayed empty-file metadata");

        auto bad{ add("folder-bad", "output/bad") };
        check(bad->m_prepared.wait_for(5s) == std::future_status::ready, "Bad preparation never completed.");
        bool reported{};
        try { message_loop(&engine, &persistent, &multi, &loads, &queue); }
        catch (const nlohmann::json::exception&) { reported = true; }
        check(reported && loads.m_end.empty(), "Preparation error was ignored or scheduled invalid work.");
        column->remove_object(bad->m_progress_div);
        loads.m_task_list.erase(bad->m_self_i);
        std::println("PASS: folder preparation errors are consumed before inspecting unfinished task state");
        engine.flush();
        gui.clear_state();
        column.reset();
        loads.m_progress_column.reset();
        lgo::text_line_t::free();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::println("FAIL: {}", error.what());
        return 1;
    }
}
