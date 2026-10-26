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
        if (const auto* protocol{ std::getenv("STORAGE_DOWNLOAD_PROTOCOL_ONLY") })
        {
            const bool background{ std::string_view{ protocol } == "worker" };
            curl_multi_worker_t worker;
            for (const auto& kind : { "missing-id", "unknown-id", "duplicate-id", "truncated", "boundary", "http-header", "http-status", "mixed" })
            {
                google_batch_t batch{};
                batch.boundary_set("+");
                batch.path_set("/drive/v3");
                for (const auto& suffix : { "a", "b" })
                    blob_get(batch, std::string{ "protocol-" } + (background ? "worker-" : "") + kind + "-" + suffix, "description");
                batch.method_post();
                if (background)
                {
                    worker.add(std::move(batch));
                    const auto deadline{ std::chrono::steady_clock::now() + 5s };
                    while (!worker.ready_nonempty())
                    {
                        worker.run();
                        check(std::chrono::steady_clock::now() < deadline, "Background batch retry stalled.");
                        std::this_thread::sleep_for(1ms);
                    }
                    auto response{ worker.ready_get_unique() };
                    batch = std::move(dynamic_cast<google_batch_t&>(*response));
                }
                else batch.run();
                check(batch.part_count() == 2, "Malformed batch added unexpected responses.");
                for (std::size_t i{}; i < 2; ++i)
                {
                    const auto payload = nlohmann::json::parse(batch.response_get(i).body_get());
                    check(payload["description"] == "[]", "Batch retry lost a response.");
                }
            }
            std::println("PASS: {} malformed multipart responses retry safely and retain successful subrequests",
                background ? "background" : "synchronous");
            return 0;
        }
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
        curl_multi_worker_t multi{};
        home(&engine, &gui, &persistent, &column, &loads, &multi, &queue, &pos);
        check(queue.count() == 0, "This test requires an empty blob reserve.");
        // A queued upload must not prevent downloads from using otherwise idle connections.
        auto blocked_upload{ std::make_shared<upload_task_t>() };
        blocked_upload->m_type = "upload";
        std::promise<void> prepared;
        prepared.set_value();
        blocked_upload->m_prepared = prepared.get_future();
        auto upload_i{ loads.m_task_list.emplace(loads.m_task_list.end(), blocked_upload) };

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
        auto reordered{ add("file-reordered", "output/reordered.bin", false) };
        // Browsing uses the GUI thread's connection while preparation uses worker-local connections.
        for (int i{}; i != 3; ++i) check(list_folder_content(google_easy_t::get(), &pos).empty(), "Browsing returned wrong data.");
        const auto deadline{ std::chrono::steady_clock::now() + 20s };
        while (loads.m_task_list.size() > 1)
        {
            check(std::chrono::steady_clock::now() < deadline, "Folder download stalled.");
            check(!message_loop(&engine, &persistent, &multi, &loads, &queue), "GUI closed during preparation/download.");
            lgo::text_line_t::pending_load();
        }
        check(loads.m_end.empty(), "A completed download still has pending callbacks.");
        check(loads.m_task_list.front() == blocked_upload && queue.count() == 0,
            "An upload ran without enough reserved blobs.");
        loads.m_task_list.erase(upload_i);
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
        auto reordered_bytes{ lgo::get_file_str("output/reordered.bin") };
        check(reordered_bytes.size() == 201, "Reordered download size differs.");
        for (std::size_t i{}; i < reordered_bytes.size(); ++i)
            check(static_cast<unsigned char>(reordered_bytes[i]) == i, "Reordered download bytes differ.");
        std::println("PASS: concurrent recursive folders, pagination, empty files/directories, delayed metadata and empty blob reserve");

        for (int i{}; i < 8; ++i)
            add("file-main", "output/unowned-" + std::to_string(i) + ".bin", false);
        const auto owner_deadline{ std::chrono::steady_clock::now() + 10s };
        while (!loads.m_task_list.empty())
        {
            check(std::chrono::steady_clock::now() < owner_deadline, "Unowned task download stalled.");
            check(!message_loop(&engine, &persistent, &multi, &loads, &queue), "GUI closed during final task release.");
            lgo::text_line_t::pending_load();
        }
        check(loads.m_end.empty(), "Last task owner was released before callbacks finished.");
        for (int i{}; i < 8; ++i)
            check(lgo::get_file_str("output/unowned-" + std::to_string(i) + ".bin") == "ABCDEF", "Unowned task content differs.");
        std::println("PASS: final callbacks safely release tasks without external owners");

        auto bad{ add("folder-bad", "output/bad") };
        check(bad->m_prepared.wait_for(5s) == std::future_status::ready, "Bad preparation never completed.");
        auto bad_index{ add("file-bad-index", "output/bad-index.bin", false) };
        auto empty_chunk{ add("file-empty-chunk", "output/empty-chunk.bin", false) };
        auto bad_content{ add("file-bad-content", "output/bad-content.bin", false) };
        auto bad_encoding{ add("file-bad-encoding", "output/bad-encoding.bin", false) };
        auto wrong_size{ add("file-wrong-size", "output/wrong-size.bin", false) };
        auto bad_output{ add("file-main", "output/bad-output.bin", false) };
        auto inflight{ add("file-failed-inflight", "output/inflight.bin", false) };
        check(bad_output->m_prepared.wait_for(5s) == std::future_status::ready, "Output preparation stalled.");
        // Force an actual output-open error after successful preparation.
        std::filesystem::remove("output/bad-output.bin");
        std::filesystem::create_directory("output/bad-output.bin");
        auto healthy{ add("file-main", "output/after-failure.bin", false) };
        const auto failures{ std::array{ bad, bad_index, empty_chunk, bad_content, bad_encoding, wrong_size, bad_output, inflight } };
        const auto error_deadline{ std::chrono::steady_clock::now() + 10s };
        while (loads.m_task_list.size() != failures.size() || !loads.m_end.empty() ||
            std::ranges::any_of(failures, [](const auto& task) { return task->m_error.empty(); }))
        {
            check(std::chrono::steady_clock::now() < error_deadline, "Failed downloads blocked other tasks.");
            check(!message_loop(&engine, &persistent, &multi, &loads, &queue), "GUI closed after download failure.");
            lgo::text_line_t::pending_load();
        }
        check(lgo::get_file_str("output/after-failure.bin") == "ABCDEF", "Healthy download did not finish after failures.");
        for (const auto& task : failures)
            check(task->m_progress_text->get_text().starts_with("下載失敗："), "Failure reason is missing from the GUI.");
        check(inflight->m_file_arr[0].m_chunk_load_begin_progress == 101 &&
            inflight->m_file_arr[0].m_chunk_load_end_progress == 0 &&
            inflight->m_file_arr[0].m_written == 0, "Failed task processed late responses or wrote content.");
        check(bad_encoding->m_error.find("Base64URL") != std::string::npos, "Invalid encoding was silently decoded.");
        std::println("PASS: preparation, metadata, empty chunk, content, encoding, size and output errors stay visible; late responses drain while other downloads finish");
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
