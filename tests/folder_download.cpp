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
        for (int i{};i!=3;++i) {
            if(!list_folder_content(google_easy_t::get(),&pos).empty()) throw std::runtime_error("Browsing returned wrong data.");
            engine.message_loop();
        }
        for(auto task : {nested,concurrent,empty,zeros,empty_file}) {
            check(task->m_prepared.wait_for(5s)==std::future_status::ready,"Preparation stalled.");
            task->m_prepared.get();
            column->remove_object(task->m_progress_div);
            loads.m_task_list.erase(task->m_self_i);
        }
        check(nested->m_file_arr.size()==4 && concurrent->m_file_arr.size()==4,"Folder entries differ.");
        std::println("PASS: production callbacks prepare concurrent folders using worker-owned HTTP handles");
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
