#include <vulkan/vk_platform.h>

import std;
import external.Vulkan;
import external.GLFW;
import lgo.io;
import lgo.math;
import lgo.GUI;
import storage.google;
import storage.kernel;

using namespace std::chrono_literals;

auto check(bool condition, const char* message) -> void
{
    if (!condition) throw std::runtime_error{ message };
}

// Inject through Vulkan-Hpp's dispatch table, without adding production fault switches.
// Drain the real device first: the test reports loss on an otherwise healthy driver.
#define INJECT(function) \
    auto original = dispatch->function; \
    dispatch->function = [](auto... args) \
    { \
        using result_t = decltype(original(args...)); \
        return static_cast<result_t>(-4); \
    }

auto test_shutdown(bool unwind) -> void
{
    bool original_error{};
    try
    {
        lgo::engine_t engine{ lgo::size_2D{ 1000, 700 }, "Lost GPU shutdown" };
        engine.flush();
        auto* dispatch = const_cast<std::remove_const_t<std::remove_pointer_t<
            decltype(engine.device_get().getDispatcher())>>*>(engine.device_get().getDispatcher());
        INJECT(vkDeviceWaitIdle);
        if (unwind) throw std::runtime_error{ "original error" };
    }
    catch (const std::runtime_error& error)
    {
        original_error = std::string{ error.what() } == "original error";
    }
    check(!unwind || original_error, "GPU cleanup replaced the original exception.");
}

auto test_retry(const std::string& mode) -> void
{
    lgo::engine_t engine{ lgo::size_2D{ 1000, 700 }, "GPU recovery failure" };
    unsigned attempts{};
    engine.texture_recovery_set([] {}, [&]
    {
        ++attempts;
        if (mode == "fallback" || (mode == "retry" && attempts < 3))
            throw std::runtime_error{ "injected rebuild failure" };
    });
    engine.flush();
    engine.device_lost();
    const auto deadline{ std::chrono::steady_clock::now() + 8s };
    bool closed{};
    while (!(closed = engine.message_loop()))
    {
        check(std::chrono::steady_clock::now() < deadline, "Rebuild retry limit timed out.");
        if (engine.gpu_available())
        {
            if (mode == "retry") break;
            check(mode == "repeated", "Permanent rebuild failure was ignored.");
            engine.flush();
            engine.device_lost();
        }
        std::this_thread::sleep_for(10ms);
    }
    check(attempts == 3, "Recovery did not use the documented attempt limit.");
    check(mode == "retry" ? !closed && engine.gpu_available() : closed,
        "Unexpected recovery fallback result.");
}

auto test_transfer_shutdown() -> void
{
    std::atomic_bool close{ true }; // Never start cloud requests in this fixture.
    std::ofstream("shutdown-queue") << "[\"unused-blob\"]";
    {
        blob_queue_t queue{ "shutdown-queue", &google_account_t::default_get(), &close };
        auto single = std::async(std::launch::async, [&] { return queue.get(); });
        auto multiple = std::async(std::launch::async, [&] { return queue.get(2); });
        check(single.wait_for(50ms) == std::future_status::timeout, "Queue fixture did not block.");
        queue.stop();
        check(single.wait_for(1s) == std::future_status::ready &&
            multiple.wait_for(1s) == std::future_status::ready, "Queue shutdown did not wake preparation workers.");
        unsigned cancelled{};
        try { (void)single.get(); } catch (const std::runtime_error&) { ++cancelled; }
        try { (void)multiple.get(); } catch (const std::runtime_error&) { ++cancelled; }
        check(cancelled == 2, "Stopped queue handed out unused blobs.");
    }
    check(lgo::get_file_str("shutdown-queue") == "[\"unused-blob\"]", "Shutdown lost the unused blob queue.");
    std::atomic_bool worker_done{};
    bool destroyed_too_early{};
    struct entity_t : upload_entity_t
    {
        std::atomic_bool* done{};
        bool* early{};
        ~entity_t() override { *early = !*done; }
    };
    {
        auto task = std::make_unique<upload_task_t>();
        auto entity = std::make_shared<entity_t>();
        entity->done = &worker_done;
        entity->early = &destroyed_too_early;
        task->m_entity_arr.push_back(std::move(entity));
        task->m_prepared = std::async(std::launch::async, [&]
        {
            std::this_thread::sleep_for(100ms);
            worker_done = true;
        });
    }
    check(worker_done && !destroyed_too_early, "Task containers were destroyed before preparation finished.");
    std::filesystem::remove("shutdown-queue");
}

int main(int argc, char** argv)
{
    std::set_terminate([] { std::_Exit(42); });
    try
    {
        const std::string mode{ argc > 1 ? argv[1] : "fence" };
        lgo::init_logfile("logfile/device-loss.txt");
        if (mode == "transfer-shutdown")
        {
            test_transfer_shutdown();
            std::println("PASS: cancelled preparation workers and saved unused blob queue");
            return 0;
        }
        if (mode == "shutdown" || mode == "unwind")
        {
            test_shutdown(mode == "unwind");
            std::println("PASS: lost-device {}", mode);
            return 0;
        }
        if (mode == "retry" || mode == "fallback" || mode == "repeated")
        {
            test_retry(mode);
            std::println("PASS: bounded GPU recovery {}", mode);
            return 0;
        }
        lgo::FreeType_t freetype{};
        lgo::font_face_t::freetype_set(&freetype);
        lgo::font_face_t face{ "font/NotoSansTC-VariableFont_wght.ttf", 24 };
        lgo::font_preference_t::default_get().push_back(&face);
        lgo::engine_t engine{ lgo::size_2D{ 1000, 700 }, "GPU recovery regression" };
        lgo::depth_tracker_t depth{};
        lgo::state_t state{ &engine, &depth };
        auto label = state.add_object_shared<lgo::text_line_std_t>
        (
            lgo::depth_range_t{ 0, 0.5 }, lgo::pos_2D{ 20, 20 }, "Transfer 25% 傳輸中",
            lgo::std_white, lgo::size_1D{ 600 }
        );
        engine.message_loop();
        engine.flush();
        auto* window{ engine.window_get() };
        auto* identity{ label.get() };
        auto* dispatch = const_cast<std::remove_const_t<std::remove_pointer_t<
            decltype(engine.device_get().getDispatcher())>>*>(engine.device_get().getDispatcher());
        if (mode == "flush") { INJECT(vkQueueWaitIdle); engine.flush(); }
        else if (mode == "fence") { INJECT(vkWaitForFences); engine.draw(); }
        else if (mode == "acquire") { INJECT(vkAcquireNextImageKHR); engine.draw(); }
        else if (mode == "submit") { INJECT(vkQueueSubmit); engine.draw(); }
        else if (mode == "present")
        {
            // Submission has already happened by present(). Complete it before simulated loss.
            auto original = dispatch->vkQueuePresentKHR;
            auto wait = dispatch->vkQueueWaitIdle;
            static decltype(wait) wait_idle{};
            wait_idle = wait;
            dispatch->vkQueuePresentKHR = [](auto queue, auto info)
            {
                wait_idle(queue);
                using result_t = decltype(original(queue, info));
                return static_cast<result_t>(-4);
            };
            engine.draw();
        }
        else if (mode == "map") { INJECT(vkMapMemory); label->set_color(lgo::std_green_light); }
        else if (mode == "buffer") { INJECT(vkCreateBuffer); label->set_text("Transfer 30% 傳輸中"); }
        else if (mode == "glyph") { INJECT(vkQueueSubmit); label->set_text("New glyph 新字"); }
        else if (mode == "native")
        {
            auto* key = engine.callback_key_add([](int, int, int) { throw vk::DeviceLostError{ "injected native callback" }; });
            auto callback = glfwSetKeyCallback(window, nullptr);
            glfwSetKeyCallback(window, callback);
            callback(window, GLFW_KEY_F3, 0, GLFW_PRESS, 0);
            engine.callback_key_remove(key);
        }
        else throw std::runtime_error{ "Unknown test mode." };
        check(!engine.gpu_available(), "Device loss was not recorded.");
        label->set_text("Transfer 75% 傳輸繼續", false);
        // pending_load must return while the device is unavailable, without spinning.
        lgo::text_line_t::pending_load();
        const auto deadline{ std::chrono::steady_clock::now() + 10s };
        do
        {
            check(!engine.message_loop(), "Recovery closed the window.");
            lgo::text_line_t::pending_load();
            check(std::chrono::steady_clock::now() < deadline, "Recovery timed out.");
        } while (!engine.gpu_available());
        check(window == engine.window_get() && identity == label.get(), "Recovery replaced CPU GUI state.");
        check(label->get_text() == "Transfer 75% 傳輸繼續", "Recovery lost the progress update.");
        for (int i{}; i != 5; ++i) engine.message_loop();
        check(engine.frame_rate() != 0, "The recovered device did not present frames.");
        engine.flush();
        state.clear_state();
        label.reset();
        lgo::text_line_t::free();
        std::println("PASS: GPU recovery at {} preserves GUI state and resumes presentation", mode);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::println("FAIL: {}", error.what());
        return 1;
    }
}
