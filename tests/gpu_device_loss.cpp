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

auto test_names() -> void
{
    lgo::FreeType_t freetype{};
    lgo::font_face_t::freetype_set(&freetype);
    lgo::font_face_t face{ "font/NotoSansTC-VariableFont_wght.ttf", 24 };
    lgo::font_preference_t::default_get().push_back(&face);
    lgo::engine_t engine{ lgo::size_2D{ 1000, 700 }, "Vulkan names" };
    engine.flush();
    auto& device{ engine.device_get() };
    auto* dispatch = const_cast<std::remove_const_t<std::remove_pointer_t<decltype(device.getDispatcher())>>*>(device.getDispatcher());
    static decltype(dispatch->vkSetDebugUtilsObjectNameEXT) original_name{};
    static std::vector<std::pair<vk::ObjectType, std::string>> names;
    original_name = dispatch->vkSetDebugUtilsObjectNameEXT;
    if constexpr (lgo::vulkan_names_enabled)
        check(original_name != nullptr, "Mock driver must support VK_EXT_debug_utils for this test.");
    static decltype(original_name) spy = [](auto device, auto info)
    {
        names.emplace_back(static_cast<vk::ObjectType>(info->objectType), info->pObjectName);
        using result_t = decltype(original_name(device, info));
        return original_name ? original_name(device, info) : static_cast<result_t>(0);
    };
    dispatch->vkSetDebugUtilsObjectNameEXT = spy;
    bool built_name{};
    const bool named{ lgo::vulkan_name(device, *device, [&] { built_name = true; return "probe.device"; }) };
    check(named == lgo::vulkan_names_enabled && built_name == lgo::vulkan_names_enabled,
        "Disabled naming built strings or enabled naming did not run.");
    dispatch->vkSetDebugUtilsObjectNameEXT = nullptr;
    built_name = false;
    check(!lgo::vulkan_name(device, *device, [&] { built_name = true; return "unavailable"; }) && !built_name,
        "Unavailable debug extension was called.");
    dispatch->vkSetDebugUtilsObjectNameEXT = [](auto... args)
    {
        using result_t = decltype(original_name(args...));
        return static_cast<result_t>(-1);
    };
    check(!lgo::vulkan_name(device, *device, "failed-name"), "Naming failure was not contained.");
    dispatch->vkSetDebugUtilsObjectNameEXT = spy;

    // New device dispatch tables obtain commands through the instance dispatcher.
    // Intercept lookup so the test observes initial names on the replacement device.
    auto* instance = const_cast<std::remove_const_t<std::remove_pointer_t<
        decltype(engine.device_physical_get().getDispatcher())>>*>(engine.device_physical_get().getDispatcher());
    static decltype(instance->vkGetDeviceProcAddr) original_get_proc{};
    original_get_proc = instance->vkGetDeviceProcAddr;
    static decltype(original_get_proc) get_proc_spy = [](auto device, auto name)
    {
        using result_t = decltype(original_get_proc(device, name));
        // Vulkan-Hpp resolves vkGetDeviceProcAddr itself before other commands.
        // Preserve interception through that lookup on replacement devices.
        if (std::string_view{ name } == "vkGetDeviceProcAddr") return reinterpret_cast<result_t>(get_proc_spy);
        if (std::string_view{ name } == "vkSetDebugUtilsObjectNameEXT") return reinterpret_cast<result_t>(spy);
        return original_get_proc(device, name);
    };
    instance->vkGetDeviceProcAddr = get_proc_spy;
    engine.device_lost();
    const auto deadline{ std::chrono::steady_clock::now() + 10s };
    do
    {
        check(!engine.message_loop(), "Naming prevented device recovery.");
        check(std::chrono::steady_clock::now() < deadline, "Naming recovery timed out.");
    } while (!engine.gpu_available());
    instance->vkGetDeviceProcAddr = original_get_proc;
    lgo::depth_tracker_t depth{};
    lgo::state_t state{ &engine, &depth };
    struct cleanup_t
    {
        lgo::state_t& state;
        lgo::engine_t& engine;
        ~cleanup_t()
        {
            try { engine.flush(); } catch (...) {}
            state.clear_state();
            lgo::text_line_t::free();
        }
    } cleanup{ state, engine };
    state.add_object_shared<lgo::text_line_std_t>
    (
        lgo::depth_range_t{ 0, 0.5 }, lgo::pos_2D{ 20, 20 }, "Names 字型",
        lgo::std_white, lgo::size_1D{ 600 }
    );
    const auto before_resize{ names.size() };
    glfwSetWindowSize(engine.window_get(), 1100, 750);
    for (int i{}; i < 5; ++i) engine.message_loop();
    engine.flush();
    if constexpr (lgo::vulkan_names_enabled)
    {
        for (const auto type : { vk::ObjectType::eDevice, vk::ObjectType::eQueue, vk::ObjectType::eCommandPool,
            vk::ObjectType::eCommandBuffer, vk::ObjectType::eFence, vk::ObjectType::eSemaphore,
            vk::ObjectType::eSwapchainKHR, vk::ObjectType::eImage, vk::ObjectType::eImageView,
            vk::ObjectType::eDeviceMemory, vk::ObjectType::eBuffer, vk::ObjectType::eShaderModule,
            vk::ObjectType::ePipelineLayout, vk::ObjectType::ePipeline, vk::ObjectType::eSampler,
            vk::ObjectType::eDescriptorSetLayout, vk::ObjectType::eDescriptorPool, vk::ObjectType::eDescriptorSet })
        {
            if (!std::ranges::any_of(names, [type](const auto& entry) { return entry.first == type; }))
            {
                std::println("Missing Vulkan object type {}", std::to_underlying(type));
                for (const auto& [named_type, name] : names)
                    std::println("Named {}: {}", std::to_underlying(named_type), name);
                throw std::runtime_error{ "A Vulkan object type was not named after recreation." };
            }
        }
        check(names.size() > before_resize, "Resized swapchain objects were not named.");
        check(std::ranges::all_of(names, [](const auto& entry) { return entry.second.find(" [0x") != std::string::npos; }),
            "Resource names cannot distinguish repeated GUI objects.");
    }
    else check(names.empty(), "Disabled build still named Vulkan objects.");
    std::println("PASS: Vulkan names {}, extension fallback, naming errors, resize and device recreation",
        lgo::vulkan_names_enabled ? "enabled" : "disabled");
}

auto test_logging() -> void
{
    std::filesystem::remove("concurrent-log.txt");
    lgo::get_logfile_string().clear();
    lgo::init_logfile("concurrent-log.txt");
    {
        std::vector<std::jthread> workers;
        for (int worker{}; worker < 4; ++worker)
            workers.emplace_back([worker]
            {
                for (int line{}; line < 100; ++line)
                    lgo::log_file(std::format("{}:{}\n", worker, line));
            });
    }
    std::istringstream lines{ lgo::get_file_str("concurrent-log.txt") };
    std::set<std::string> entries;
    for (std::string line; std::getline(lines, line);) entries.insert(line);
    check(entries.size() == 400, "Concurrent log writes were lost or interleaved.");
    // A directory cannot be opened for appending: this used to retry forever.
    lgo::get_logfile_path() = ".";
    lgo::log_file("unwritable log probe\n");
    lgo::get_logfile_path() = "concurrent-log.txt";
    lgo::log_file("reopened\n");
    check(lgo::get_file_str("concurrent-log.txt").ends_with("reopened\n"), "Logger did not recover after an open failure.");
    std::println("PASS: concurrent log writes, unwritable destination and reopen");
}

// A CPU-only object exercises names without depending on font or GPU setup.
class naming_probe_t: public lgo::GUI_object_t
{
public:
    naming_probe_t() = default;
    naming_probe_t(lgo::engine_t*, lgo::depth_tracker_t*) {}
    auto get_pos() const -> lgo::pos_2D override { return {}; }
    auto set_pos(lgo::pos_2D) -> void override {}
    auto get_size() const -> lgo::size_2D override { return {}; }
    auto get_margin() const -> lgo::size_1D override { return {}; }
    auto log(std::uint32_t tabs) const -> void override { GUI_object_t::log(tabs); }
    auto show_impl(bool base) -> void override { if (show_begin(base)) show_end(base); }
    auto hide_impl(bool base) -> void override { if (hide_begin(base)) hide_end(base); }
};

auto test_gui_names() -> void
{
    // Capture only this test's diagnostics; logger buffers in memory before init.
    lgo::get_logfile_path().clear();
    lgo::get_logfile_string().clear();
    {
        naming_probe_t first, second;
        if constexpr (lgo::gui_object_names_enabled)
        {
            check(!first.name_get().empty() && first.name_get() != second.name_get(),
                "Automatic GUI names must be nonempty and unique.");
            check(first.name_get().find("naming_probe_t") != std::string::npos,
                "Automatic name lost its construction context.");
        }
        else check(first.name_get().empty() && second.name_get().empty(), "Disabled GUI naming generated names.");
        first.name_set("transfer progress");
        check(first.name_get() == "transfer progress", "Explicit GUI names did not work.");
        naming_probe_t copied{ first };
        if constexpr (lgo::gui_object_names_enabled)
            check(copied.name_get() != first.name_get() && copied.name_get().contains("transfer progress"),
                "A copied GUI object reused its source identity.");
        const auto destination_name{ second.name_get() };
        second = first;
        check(second.name_get() == destination_name, "Assignment replaced the destination GUI identity.");
        first.show();
        first.hide();
        if constexpr (!lgo::gui_object_events_enabled)
            check(lgo::get_logfile_string().empty(), "Naming alone emitted automatic GUI event logs.");
        first.log(0);
        lgo::state_t state;
        state.add_object<naming_probe_t>();
        state.log(0);
        state.clear_state();
        // Composite members are constructed directly, outside state_t's factory.
        lgo::button_plus_t plus;
        lgo::text_line_std_t text;
        if constexpr (lgo::gui_object_names_enabled)
            check(!plus.name_get().empty() && !text.name_get().empty(), "Composite objects were unnamed.");
        plus.log(0);
        text.log(0);
    }
    const auto& output{ lgo::get_logfile_string() };
    for (const auto marker : { "GUI create", "GUI rename", "GUI copy", "GUI show", "GUI hide", "GUI attach", "GUI destroy" })
        check(output.contains(marker) == lgo::gui_object_events_enabled, "GUI lifecycle log switch was not respected.");
    if constexpr (lgo::gui_object_names_enabled)
    {
        for (const auto marker : { "type: GUI_object_t", "type: click_area_t", "type: rect_t" })
            check(output.contains(marker), "Explicit nested object logging was missing.");
    }
    else check(output.empty(), "Disabled GUI naming still emitted object logs.");
    std::println("PASS: GUI names {}, automatic events {}, unique identities, copies, composite members and explicit logs",
        lgo::gui_object_names_enabled ? "enabled" : "disabled",
        lgo::gui_object_events_enabled ? "enabled" : "disabled");
}

auto test_capabilities() -> void
{
    vk::PhysicalDeviceMemoryProperties properties{};
    properties.memoryTypeCount = 3;
    for (std::uint32_t i{}; i < 3; ++i) properties.memoryTypes[i].propertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
    vk::MemoryRequirements requirements{ .size{ 256 }, .alignment{ 1 }, .memoryTypeBits{ 4 } };
    check(lgo::vulkan_memory_type_index(requirements, properties, vk::MemoryPropertyFlagBits::eDeviceLocal) == 2,
        "An unsupported memory type passed the type-bit mask.");
    properties.memoryTypes[2].propertyFlags = vk::MemoryPropertyFlagBits::eHostVisible;
    bool rejected{};
    try { lgo::vulkan_memory_type_index(requirements, properties, vk::MemoryPropertyFlagBits::eDeviceLocal); }
    catch (const lgo::vulkan_unsupported_t&) { rejected = true; }
    check(rejected, "Missing device memory properties were not rejected.");
    std::println("PASS: Vulkan memory type bits and required properties");
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
        if (mode == "logging-crash")
        {
            std::filesystem::remove("crash-log.txt");
            lgo::init_logfile("crash-log.txt");
            lgo::log_file("first completed message\n");
            lgo::log_file("last completed message\n");
            // _Exit skips C++ destructors and C stream flushing: the persistent
            // logger handle is deliberately left open at abrupt process exit.
            std::_Exit(86);
        }
        if (mode == "logging")
        {
            test_logging();
            return 0;
        }
        if (mode == "gui-names")
        {
            test_gui_names();
            return 0;
        }
        if (mode == "capabilities")
        {
            test_capabilities();
            return 0;
        }
        if (mode == "names")
        {
            test_names();
            return 0;
        }
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
