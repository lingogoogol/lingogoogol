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

// Widen the Vulkan calls and mapped-memory interval so missing synchronization
// reliably overlaps even without an installed Windows Vulkan validation layer.
struct gpu_concurrency_probe_t
{
    inline static std::atomic<int> active{};
    inline static std::atomic<int> mapped{};
    inline static std::atomic<int> overlaps{};
    inline static std::atomic<int> presents{};

    gpu_concurrency_probe_t()
    {
        if (active.fetch_add(1) != 0 || mapped.load() != 0) ++overlaps;
        std::this_thread::sleep_for(1ms);
    }
    ~gpu_concurrency_probe_t() { --active; }
};

auto test_rect_threads() -> void
{
    lgo::FreeType_t freetype{};
    lgo::font_face_t::freetype_set(&freetype);
    lgo::font_face_t face{ "font/NotoSansTC-VariableFont_wght.ttf", 24 };
    lgo::font_preference_t::default_get().push_back(&face);
    lgo::engine_t engine{ lgo::size_2D{ 1000, 700 }, "Concurrent rectangles" };
    engine.flush();
    auto* dispatch = const_cast<std::remove_const_t<std::remove_pointer_t<decltype(engine.device_get().getDispatcher())>>*>(engine.device_get().getDispatcher());
    static auto submit{ dispatch->vkQueueSubmit };
    static auto present{ dispatch->vkQueuePresentKHR };
    static auto wait{ dispatch->vkQueueWaitIdle };
    static auto device_wait{ dispatch->vkDeviceWaitIdle };
    static auto map{ dispatch->vkMapMemory };
    static auto unmap{ dispatch->vkUnmapMemory };
    static auto update_descriptors{ dispatch->vkUpdateDescriptorSets };
    static std::atomic<std::uint64_t> mapped_size{};
    static std::atomic<std::uintptr_t> descriptor_view{};
    dispatch->vkUpdateDescriptorSets = [](auto device, auto count, auto writes, auto copy_count, auto copies)
    {
        if (count && writes[0].pImageInfo)
            descriptor_view = std::bit_cast<std::uintptr_t>(writes[0].pImageInfo[0].imageView);
        update_descriptors(device, count, writes, copy_count, copies);
    };
    dispatch->vkQueueSubmit = [](auto... args)
    {
        gpu_concurrency_probe_t probe;
        return submit(args...);
    };
    dispatch->vkQueuePresentKHR = [](auto... args)
    {
        gpu_concurrency_probe_t probe;
        ++gpu_concurrency_probe_t::presents;
        return present(args...);
    };
    dispatch->vkQueueWaitIdle = [](auto... args)
    {
        gpu_concurrency_probe_t probe;
        return wait(args...);
    };
    dispatch->vkDeviceWaitIdle = [](auto... args)
    {
        gpu_concurrency_probe_t probe;
        return device_wait(args...);
    };
    dispatch->vkMapMemory = [](auto device, auto memory, auto offset, auto size, auto flags, auto data)
    {
        gpu_concurrency_probe_t probe;
        const auto result{ map(device, memory, offset, size, flags, data) };
        if (static_cast<int>(result) == 0)
        {
            ++gpu_concurrency_probe_t::mapped;
            mapped_size = size;
        }
        return result;
    };
    dispatch->vkUnmapMemory = [](auto... args)
    {
        // Include CPU writes between map and unmap, not only the API calls.
        std::this_thread::sleep_for(1ms);
        unmap(args...);
        --gpu_concurrency_probe_t::mapped;
    };

    std::array<lgo::image_t, 2> images;
    std::array<lgo::image_view_t, 2> views;
    {
        auto guard{ engine.gpu_lock() };
        auto commands{ engine.command_buffer_create() };
        for (std::size_t i{}; i < images.size(); ++i)
        {
            lgo::pixmap_t pixels;
            pixels.dim_set({ 1, 1 });
            pixels[0, 0] = i == 0 ? std::byte{ 0 } : std::byte{ 255 };
            images[i].init({ .name{ "thread test texture" },
                .device_physical{ &engine.device_physical_get() }, .device{ &engine.device_get() },
                .command_buffer{ &commands }, .pixmap{ &pixels } });
            views[i].init({ .name{ "thread test texture" }, .device{ &engine.device_get() },
                .image{ &images[i].interface_get() }, .format{ vk::Format::eR8Unorm },
                .aspect{ vk::ImageAspectFlagBits::eColor } });
        }
        commands.end();
        engine.command_queue_get().submit(vk::SubmitInfo{ .commandBufferCount{ 1 }, .pCommandBuffers{ &*commands } }, nullptr);
        engine.flush();
    }
    auto* shared{ engine.add_rect({ .pos{ 20, 20 }, .size{ 100, 100 }, .depth{ 0.5f }, .color{ lgo::std_white },
        .image_view{ &views[0] }, .texture_axis_x{ 100, 0 }, .texture_axis_y{ 0, 100 } }) };
    auto* separate{ engine.add_rect({ .pos{ 200, 20 }, .size{ 100, 100 }, .depth{ 0.5f }, .color{ lgo::std_white } }) };
    lgo::depth_tracker_t depth;
    lgo::state_t state{ &engine, &depth };
    struct cleanup_t
    {
        lgo::engine_t& engine;
        lgo::state_t& state;
        lgo::rect_primitive_t* shared;
        lgo::rect_primitive_t* separate;
        ~cleanup_t()
        {
            engine.flush();
            state.clear_state();
            lgo::text_line_t::free();
            engine.remove_rect(shared);
            engine.remove_rect(separate);
        }
    } cleanup{ engine, state, shared, separate };
    auto label{ state.add_object_shared<lgo::text_line_std_t>(lgo::depth_range_t{ 0, 0.4 },
        lgo::pos_2D{ 20, 300 }, "Uploads", lgo::std_white, lgo::size_1D{ 600 }) };
    std::atomic<int> ready{}, done{};
    std::atomic<bool> start{};
    std::array<std::exception_ptr, 3> errors{};
    std::vector<std::jthread> workers;
    for (int worker{}; worker < 3; ++worker)
        workers.emplace_back([&, worker]
        {
            ++ready;
            while (!start.load()) std::this_thread::yield();
            try
            {
                for (int i{}; i < 60; ++i)
                {
                    auto* rect{ worker == 2 ? separate : shared };
                    if (worker == 0)
                    {
                        rect->set_pos({ 20 + i, 20 + i });
                        rect->set_size({ 100 + i, 100 + i });
                    }
                    else if (worker == 1)
                    {
                        rect->set_color(i % 2 ? lgo::std_white : lgo::std_green_light);
                        rect->set_image_view(views[i % 2]);
                        rect->color_enable();
                        rect->texture_enable();
                    }
                    else
                    {
                        rect->set_pos({ 250 + i, 20 });
                        rect->set_color(lgo::std_green_light);
                        engine.flush();
                        rect->log(0);
                    }
                    rect->inside({ 50, 50 });
                    std::this_thread::sleep_for(1ms);
                }
            }
            catch (...) { errors[worker] = std::current_exception(); }
            ++done;
        });
    while (ready.load() != 3) std::this_thread::yield();
    start = true;
    int frames{};
    const auto deadline{ std::chrono::steady_clock::now() + 30s };
    while (done.load() != 3 || frames < 30)
    {
        check(std::chrono::steady_clock::now() < deadline, "Concurrent rectangle updates deadlocked.");
        if (frames == 5) glfwSetWindowSize(engine.window_get(), 1100, 750);
        if (frames == 10) label->set_text("Glyph upload while rectangles change 字型");
        check(!engine.message_loop(), "Concurrent updates closed the GUI.");
        ++frames;
    }
    workers.clear();
    for (auto error : errors) if (error) std::rethrow_exception(error);
    engine.flush();
    shared->set_image_view(views[0]);
    const auto first_view{ descriptor_view.load() };
    shared->set_image_view(views[1]);
    check(descriptor_view != first_view, "Replacing the image view did not change its descriptor.");
    const auto textured_size{ mapped_size.load() };
    shared->color_enable();
    check(mapped_size != textured_size, "Switching to color kept the textured vertex layout.");
    check(!engine.message_loop(), "Rendering the solid vertex layout failed.");
    shared->texture_enable();
    check(mapped_size == textured_size, "Switching to texture did not restore the textured vertex layout.");
    check(!engine.message_loop(), "Rendering the textured vertex layout failed.");
    check(gpu_concurrency_probe_t::overlaps == 0, "Queue operations or GPU submissions overlapped CPU vertex writes.");
    check(gpu_concurrency_probe_t::mapped == 0, "Vertex memory remained mapped.");
    check(gpu_concurrency_probe_t::presents >= 30, "Concurrent updates prevented presentation.");
    check(shared->inside({ 80, 80 }) && !shared->inside({ 78, 78 }), "Final shared rectangle position was lost.");
    check(separate->inside({ 310, 21 }) && !separate->inside({ 308, 21 }), "Final separate rectangle position was lost.");
    check(engine.gpu_available(), "Concurrent updates lost the GPU.");
    engine.set_exit();
    check(engine.message_loop(), "The GUI could not close after concurrent updates.");
    check(!lgo::get_file_str(lgo::get_logfile_path()).contains("uncaught exception:"), "Concurrent updates logged an exception.");
    lgo::log_file("finished_all!\n");
    std::println("PASS: concurrent setters on shared/separate rectangles, queue waits, mode/view changes, glyph uploads, resize and clean shutdown");
}

auto test_rect_threads_recovery() -> void
{
    lgo::engine_t engine{ lgo::size_2D{ 1000, 700 }, "Concurrent rectangle recovery" };
    auto* rect{ engine.add_rect({ .pos{ 20, 20 }, .size{ 100, 100 }, .depth{ 0.5f }, .color{ lgo::std_white } }) };
    std::atomic<int> updates{};
    std::atomic<bool> stopped{};
    std::array<std::exception_ptr, 2> errors{};
    std::vector<std::jthread> workers;
    for (int worker{}; worker < 2; ++worker)
        workers.emplace_back([&, worker](std::stop_token stop)
        {
            try
            {
                while (!stop.stop_requested())
                {
                    if (worker == 0) rect->set_pos({ 20 + updates.load() % 100, 20 });
                    else rect->set_color(lgo::std_green_light);
                    ++updates;
                    std::this_thread::sleep_for(1ms);
                }
            }
            catch (...) { errors[worker] = std::current_exception(); stopped = true; }
        });
    const auto deadline{ std::chrono::steady_clock::now() + 20s };
    for (int recovery{}; recovery < 2; ++recovery)
    {
        const auto target{ updates.load() + 20 };
        while (updates.load() < target)
        {
            check(!stopped && std::chrono::steady_clock::now() < deadline, "Rectangle workers stopped during GPU recovery.");
            check(!engine.message_loop(), "The GUI closed during concurrent recovery.");
        }
        engine.device_lost();
        // Logical edits made while the device is unavailable must survive rebuild.
        rect->set_size({ 150 + recovery, 150 + recovery });
        check(!engine.message_loop() && engine.gpu_available(), "Concurrent rectangle updates prevented device recovery.");
    }
    for (auto& worker : workers) worker.request_stop();
    workers.clear();
    for (auto error : errors) if (error) std::rethrow_exception(error);
    rect->set_pos({ 50, 50 });
    check(rect->inside({ 200, 200 }) && !rect->inside({ 201, 201 }), "Device recovery lost logical rectangle edits.");
    for (int i{}; i < 5; ++i) check(!engine.message_loop(), "Recovered rectangles could not render.");
    engine.remove_rect(rect);
    engine.set_exit();
    check(engine.message_loop(), "Concurrent recovery prevented clean shutdown.");
    std::println("PASS: active rectangle workers survive two GPU rebuilds and preserve logical edits");
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
    static std::uint64_t instance_handle;
    original_name = dispatch->vkSetDebugUtilsObjectNameEXT;
    if constexpr (lgo::vulkan_names_enabled)
        check(original_name != nullptr, "Mock driver must support VK_EXT_debug_utils for this test.");
    static decltype(original_name) spy = [](auto device, auto info)
    {
        names.emplace_back(static_cast<vk::ObjectType>(info->objectType), info->pObjectName);
        if (static_cast<vk::ObjectType>(info->objectType) == vk::ObjectType::eInstance) instance_handle = info->objectHandle;
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
    check(!lgo::vulkan_name(device, *device, []() -> std::string { throw std::runtime_error{ "name builder failed" }; }),
        "Diagnostic string construction interrupted rendering.");

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
    #if defined(_DEBUG) && defined(DEBUG_VULKAN_NAME)
    check(instance_handle != 0, "The instance name was not captured.");
    // Send a real error through the engine's driver callback. A null optional
    // message ID must be safe; the former bare throw terminated this process.
    const vk::Instance named_instance{ reinterpret_cast<vk::Instance::CType>(static_cast<std::uintptr_t>(instance_handle)) };
    named_instance.submitDebugUtilsMessageEXT(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral,
        vk::DebugUtilsMessengerCallbackDataEXT{ .pMessage{ "synthetic validation callback error" } }, *instance);
    check(lgo::get_file_str(lgo::get_logfile_path()).contains("synthetic validation callback error"),
        "The driver callback did not record the validation error.");
    #endif
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
    rejected = false;
    try { lgo::vulkan_surface_format({}); }
    catch (const lgo::vulkan_unsupported_t&) { rejected = true; }
    check(rejected, "An empty surface format list was accepted.");
    std::array formats{ vk::SurfaceFormatKHR{ .format{ vk::Format::eUndefined }, .colorSpace{ vk::ColorSpaceKHR::eSrgbNonlinear } } };
    check(lgo::vulkan_surface_format(formats).format == vk::Format::eB8G8R8A8Srgb,
        "Undefined surface format did not permit application selection.");
    formats[0].format = vk::Format::eR8G8B8A8Unorm;
    check(lgo::vulkan_surface_format(formats).format == vk::Format::eR8G8B8A8Unorm,
        "Supported RGBA fallback format was rejected.");
    formats[0].format = vk::Format::eR16G16B16A16Sfloat;
    rejected = false;
    try { lgo::vulkan_surface_format(formats); }
    catch (const lgo::vulkan_unsupported_t&) { rejected = true; }
    check(rejected, "An unsupported HDR format was silently accepted.");
    check(lgo::vulkan_composite_alpha(vk::CompositeAlphaFlagBitsKHR::eInherit) == vk::CompositeAlphaFlagBitsKHR::eInherit,
        "A surface without opaque composition had no fallback.");
    rejected = false;
    try { lgo::vulkan_composite_alpha({}); }
    catch (const lgo::vulkan_unsupported_t&) { rejected = true; }
    check(rejected, "An empty alpha capability mask was accepted.");
    std::println("PASS: memory type bits/properties, empty/undefined/SDR/HDR surface formats and alpha fallback");
}

auto test_fatal_cleanup(const std::string& mode) -> void
{
    static int destroyed_buffers;
    static bool destroyed_before_device;
    bool reported{};
    try
    {
        lgo::engine_t engine{ lgo::size_2D{ 1000, 700 }, "Fatal Vulkan cleanup" };
        engine.flush();
        auto* dispatch = const_cast<std::remove_const_t<std::remove_pointer_t<decltype(engine.device_get().getDispatcher())>>*>(engine.device_get().getDispatcher());
        static decltype(dispatch->vkDestroyBuffer) destroy_buffer;
        static decltype(dispatch->vkDestroyDevice) destroy_device;
        destroy_buffer = dispatch->vkDestroyBuffer;
        destroy_device = dispatch->vkDestroyDevice;
        dispatch->vkDestroyBuffer = [](auto... args) { ++destroyed_buffers; destroy_buffer(args...); };
        dispatch->vkDestroyDevice = [](auto... args)
        {
            destroyed_before_device = destroyed_buffers > 0;
            destroy_device(args...);
        };
        auto* rect = engine.add_rect({ .pos{ 10, 10 }, .size{ 100, 80 }, .depth{ 0.1f }, .color{ lgo::std_white } });
        if (mode == "fatal-remove")
        {
            auto original = dispatch->vkQueueWaitIdle;
            dispatch->vkQueueWaitIdle = [](auto... args) { return static_cast<decltype(original(args...))>(-2); };
            engine.remove_rect(rect);
        }
        else
        {
            static int code;
            code = mode == "fatal-unknown" ? -13 : -2;
            auto original = dispatch->vkQueueSubmit;
            dispatch->vkQueueSubmit = [](auto... args) { return static_cast<decltype(original(args...))>(code); };
            engine.draw();
        }
    }
    catch (const vk::SystemError& error)
    {
        reported = error.code().value() == (mode == "fatal-unknown" ? -13 : -2);
    }
    check(reported && destroyed_before_device, "Fatal Vulkan errors lost the cause or leaked children beyond device destruction.");
    std::println("PASS: {} reports the API error and destroys tracked buffers before the GPU device", mode);
}

auto test_shader_failure() -> void
{
    const auto original{ lgo::get_file_str("shader/solid.spv") };
    struct restore_t
    {
        const std::string& bytes;
        ~restore_t() { lgo::create_ofstream_destroy("shader/solid.spv") << bytes; }
    } restore{ original };
    lgo::create_ofstream_destroy("shader/solid.spv") << "bad";
    bool rejected{};
    try { lgo::engine_t failed{ lgo::size_2D{ 1000, 700 }, "Invalid shader" }; }
    catch (const std::runtime_error& error) { rejected = std::string_view{ error.what() }.contains("SPIR-V"); }
    check(rejected, "A truncated shader was passed to the driver.");
    lgo::create_ofstream_destroy("shader/solid.spv") << std::string(20, 'x');
    rejected = false;
    try { lgo::engine_t failed{ lgo::size_2D{ 1000, 700 }, "Invalid shader magic" }; }
    catch (const std::runtime_error& error) { rejected = std::string_view{ error.what() }.contains("magic"); }
    check(rejected, "An invalid SPIR-V header was passed to the driver.");
    lgo::create_ofstream_destroy("shader/solid.spv") << original;
    lgo::engine_t recovered{ lgo::size_2D{ 1000, 700 }, "Shader restored" };
    recovered.message_loop();
    check(recovered.frame_rate() > 0, "Constructor failure leaked native or static GPU state.");
    std::println("PASS: corrupt shader diagnostics and cleanup after failed initialization");
}

auto test_vulkan_errors(const std::string& mode) -> void
{
    lgo::engine_t engine{ lgo::size_2D{ 1000, 700 }, "Vulkan errors" };
    engine.flush();
    auto* dispatch = const_cast<std::remove_const_t<std::remove_pointer_t<decltype(engine.device_get().getDispatcher())>>*>(engine.device_get().getDispatcher());
    if (mode == "device-fallback")
    {
        auto* instance = const_cast<std::remove_const_t<std::remove_pointer_t<
            decltype(engine.device_physical_get().getDispatcher())>>*>(engine.device_physical_get().getDispatcher());
        static auto physical = static_cast<vk::PhysicalDevice::CType>(*engine.device_physical_get());
        static decltype(instance->vkEnumeratePhysicalDevices) original_enumerate;
        original_enumerate = instance->vkEnumeratePhysicalDevices;
        instance->vkEnumeratePhysicalDevices = [](auto instance, auto* count, auto* devices)
        {
            if (devices) { devices[0] = physical; devices[1] = physical; }
            *count = 2;
            return static_cast<decltype(original_enumerate(instance, count, devices))>(0);
        };
        static decltype(instance->vkCreateDevice) original_create;
        original_create = instance->vkCreateDevice;
        static int attempts;
        instance->vkCreateDevice = [](auto... args)
        {
            if (++attempts == 1) return static_cast<decltype(original_create(args...))>(-3);
            return original_create(args...);
        };
        engine.device_lost();
        check(!engine.message_loop() && engine.gpu_available(), "GPU initialization did not try the next candidate.");
        check(attempts == 2, "GPU fallback did not attempt both candidates.");
        instance->vkEnumeratePhysicalDevices = original_enumerate;
        instance->vkCreateDevice = original_create;
    }
    else if (mode == "fence-timeout")
    {
        auto original = dispatch->vkWaitForFences;
        dispatch->vkWaitForFences = [](auto... args) { return static_cast<decltype(original(args...))>(2); };
        engine.draw();
        dispatch->vkWaitForFences = original;
        check(engine.gpu_available(), "A fence timeout incorrectly discarded the GPU.");
    }
    else if (mode == "acquire-suboptimal")
    {
        static decltype(dispatch->vkAcquireNextImageKHR) original;
        original = dispatch->vkAcquireNextImageKHR;
        dispatch->vkAcquireNextImageKHR = [](auto... args)
        {
            const auto result{ original(args...) };
            return static_cast<int>(result) == 0 ? static_cast<decltype(result)>(1000001003) : result;
        };
        engine.draw();
        dispatch->vkAcquireNextImageKHR = original;
    }
    else if (mode == "acquire-timeout" || mode == "acquire-not-ready" || mode == "acquire-outdated" || mode == "surface-lost")
    {
        static int code;
        code = mode == "acquire-timeout" ? 2 : mode == "acquire-not-ready" ? 1 : mode == "acquire-outdated" ? -1000001004 : -1000000000;
        auto original = dispatch->vkAcquireNextImageKHR;
        dispatch->vkAcquireNextImageKHR = [](auto... args) { return static_cast<decltype(original(args...))>(code); };
        engine.draw();
        dispatch->vkAcquireNextImageKHR = original;
        if (mode == "surface-lost") check(!engine.gpu_available(), "Lost surface did not schedule GPU recovery.");
    }
    else if (mode == "present-outdated" || mode == "present-surface-lost" || mode == "present-suboptimal")
    {
        static int code;
        code = mode == "present-outdated" ? -1000001004 : mode == "present-suboptimal" ? 1000001003 : -1000000000;
        static decltype(dispatch->vkQueuePresentKHR) original;
        original = dispatch->vkQueuePresentKHR;
        dispatch->vkQueuePresentKHR = [](auto... args)
        {
            // Consume the real present wait before injecting a result on a healthy
            // device; otherwise a signaled semaphore is left pending in the mock.
            original(args...);
            return static_cast<decltype(original(args...))>(code);
        };
        engine.draw();
        dispatch->vkQueuePresentKHR = original;
    }
    else if (mode == "out-of-memory")
    {
        auto original = dispatch->vkCreateBuffer;
        dispatch->vkCreateBuffer = [](auto... args) { return static_cast<decltype(original(args...))>(-2); };
        bool rejected{};
        try
        {
            lgo::buffer_t buffer{ { .device_physical{ &engine.device_physical_get() }, .device{ &engine.device_get() },
                .usage{ vk::BufferUsageFlagBits::eVertexBuffer }, .size{ 128 } } };
        }
        catch (const vk::OutOfDeviceMemoryError&) { rejected = true; }
        dispatch->vkCreateBuffer = original;
        check(rejected, "GPU allocation failure was swallowed.");
    }
    else if (mode == "depth-fallback" || mode == "no-depth-format" || mode == "no-surface-formats" ||
        mode == "no-present-modes" || mode == "no-device-features" || mode == "image-memory-properties")
    {
        auto* instance = const_cast<std::remove_const_t<std::remove_pointer_t<
            decltype(engine.device_physical_get().getDispatcher())>>*>(engine.device_physical_get().getDispatcher());
        static decltype(instance->vkGetPhysicalDeviceFormatProperties) original_properties;
        static decltype(instance->vkGetPhysicalDeviceFeatures2) original_features;
        static decltype(instance->vkGetPhysicalDeviceMemoryProperties) original_memory;
        original_properties = instance->vkGetPhysicalDeviceFormatProperties;
        original_features = instance->vkGetPhysicalDeviceFeatures2;
        original_memory = instance->vkGetPhysicalDeviceMemoryProperties;
        auto original_formats = instance->vkGetPhysicalDeviceSurfaceFormatsKHR;
        auto original_modes = instance->vkGetPhysicalDeviceSurfacePresentModesKHR;
        static bool no_depth;
        no_depth = mode == "no-depth-format";
        if (mode == "depth-fallback" || no_depth)
            instance->vkGetPhysicalDeviceFormatProperties = [](auto physical, auto format, auto* properties)
            {
                original_properties(physical, format, properties);
                if (static_cast<vk::Format>(format) == vk::Format::eD32Sfloat ||
                    (no_depth && static_cast<vk::Format>(format) == vk::Format::eD16Unorm)) properties->optimalTilingFeatures = 0;
            };
        else if (mode == "no-surface-formats")
            instance->vkGetPhysicalDeviceSurfaceFormatsKHR = [](auto, auto, auto* count, auto*)
            {
                *count = 0;
                return static_cast<decltype(original_formats(nullptr, {}, nullptr, nullptr))>(0);
            };
        else if (mode == "no-present-modes")
            instance->vkGetPhysicalDeviceSurfacePresentModesKHR = [](auto, auto, auto* count, auto*)
            {
                *count = 0;
                return static_cast<decltype(original_modes(nullptr, {}, nullptr, nullptr))>(0);
            };
        else if (mode == "no-device-features")
            instance->vkGetPhysicalDeviceFeatures2 = [](auto physical, auto* features)
            {
                original_features(physical, features);
                for (auto* chain = reinterpret_cast<vk::BaseOutStructure*>(features->pNext); chain; chain = chain->pNext)
                    if (chain->sType == vk::StructureType::ePhysicalDeviceVulkan13Features)
                        reinterpret_cast<vk::PhysicalDeviceVulkan13Features*>(chain)->dynamicRendering = vk::False;
            };
        else
            instance->vkGetPhysicalDeviceMemoryProperties = [](auto physical, auto* properties)
            {
                original_memory(physical, properties);
                for (std::uint32_t i{}; i < properties->memoryTypeCount; ++i) properties->memoryTypes[i].propertyFlags = 0;
            };
        if (mode == "image-memory-properties")
        {
            bool rejected{};
            try
            {
                lgo::image_t image{ { .device_physical{ &engine.device_physical_get() }, .device{ &engine.device_get() },
                    .width{ 16 }, .height{ 16 }, .format{ engine.depth_format_get() },
                    .usage{ vk::ImageUsageFlagBits::eDepthStencilAttachment }, .property{ vk::MemoryPropertyFlagBits::eDeviceLocal } } };
            }
            catch (const lgo::vulkan_unsupported_t&) { rejected = true; }
            check(rejected, "Image allocation did not forward the requested memory properties.");
        }
        else
        {
            engine.device_lost();
            check(!engine.message_loop(), "Unsupported capability closed the window before retrying.");
            if (mode == "depth-fallback") check(engine.depth_format_get() == vk::Format::eD16Unorm,
                "Depth format did not fall back to D16.");
            else check(!engine.gpu_available(), "Missing Vulkan capability was treated as a successful rebuild.");
        }
        instance->vkGetPhysicalDeviceFormatProperties = original_properties;
        instance->vkGetPhysicalDeviceSurfaceFormatsKHR = original_formats;
        instance->vkGetPhysicalDeviceSurfacePresentModesKHR = original_modes;
        instance->vkGetPhysicalDeviceFeatures2 = original_features;
        instance->vkGetPhysicalDeviceMemoryProperties = original_memory;
    }
    const auto deadline{ std::chrono::steady_clock::now() + 10s };
    do
    {
        check(!engine.message_loop(), "Recoverable Vulkan error closed the window.");
        check(std::chrono::steady_clock::now() < deadline, "Vulkan error recovery timed out.");
    } while (!engine.gpu_available());
    for (int i{}; i < 4; ++i) check(!engine.message_loop(), "Recovered renderer closed.");
    check(engine.frame_rate() > 0, "Rendering did not resume after a Vulkan error.");
    std::println("PASS: Vulkan {} preserves event processing and resumes rendering", mode);
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
        if (mode == "rect-threads")
        {
            test_rect_threads();
            test_rect_threads_recovery();
            return 0;
        }
        if (mode == "fatal-submit" || mode == "fatal-remove" || mode == "fatal-unknown")
        {
            test_fatal_cleanup(mode);
            return 0;
        }
        if (mode == "invalid-shader")
        {
            test_shader_failure();
            return 0;
        }
        if (mode == "capabilities")
        {
            test_capabilities();
            return 0;
        }
        if (mode == "fence-timeout" || mode == "acquire-timeout" || mode == "acquire-not-ready" ||
            mode == "acquire-outdated" || mode == "surface-lost" || mode == "present-outdated" ||
            mode == "present-surface-lost" || mode == "depth-fallback" || mode == "no-surface-formats" || mode == "out-of-memory" || mode == "device-fallback" ||
            mode == "no-depth-format" || mode == "no-present-modes" || mode == "no-device-features" ||
            mode == "image-memory-properties" || mode == "acquire-suboptimal" || mode == "present-suboptimal")
        {
            test_vulkan_errors(mode);
            return 0;
        }
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
