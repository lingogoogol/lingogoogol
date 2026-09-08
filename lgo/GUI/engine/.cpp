module lgo.GUI.engine;

import std;

import external.Vulkan;
import external.GLFW;
import lgo.io.file;

namespace lgo {
    template<typename t_callback_set, typename t_caller, typename... t_in>
    auto engine_t::call_callback(const t_caller& caller, t_callback_set& callback, t_in... in) -> void {
        callback.m_calling_callback = true;
        ++callback.m_call_depth;
        const std::vector<typename decltype(callback.m_effective)::key_type> callbacks(
            callback.m_effective.begin(), callback.m_effective.end());
        for (auto* current : callbacks) {
            if (callback.m_effective.contains(current)) {
                try {
                    caller(current, in...);
                }
                catch (const std::exception& error) {
                    log_file(std::string{ "event callback failed: " } + error.what() + "\n");
                }
                catch (...) {
                    log_file("event callback failed with an unknown exception\n");
                }
            }
        }
        --callback.m_call_depth;
        if (!callback.m_call_depth) {
            callback.m_effective.insert(callback.m_pending.begin(), callback.m_pending.end());
            callback.m_pending.clear();
            for (auto* retired : callback.m_retired) {
                delete retired;
            }
            callback.m_retired.clear();
            callback.m_calling_callback = false;
        }
    }

    template<typename... t_arg>
    auto engine_t::remove_callback(callback_set<t_arg...>& callbacks, std::function<void(t_arg...)>* callback) -> void {
        if (!callback) {
            return;
        }
        if (callbacks.m_pending.erase(callback)) {
            delete callback;
            return;
        }
        if (!callbacks.m_effective.erase(callback)) {
            return;
        }
        if (callbacks.m_calling_callback) {
            callbacks.m_retired.emplace(callback);
        }
        else {
            delete callback;
        }
    }

    auto __stdcall engine_t::debug_callback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* data,
        void*
    ) -> vk::Bool32 {
        std::string severity_str{};
        switch (severity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: {
            severity_str = "verbose";
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: {
            severity_str = "info";
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: {
            severity_str = "warning";
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: {
            severity_str = "error";
            break;
        }
        default: {
            severity_str = "unknown";
            break;
        }
        }
        std::string type_str{};
        if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral) {
            type_str = "general";
        }
        if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation) {
            if (!type_str.empty()) type_str += "|";
            type_str += "validation";
        }
        if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance) {
            if (!type_str.empty()) type_str += "|";
            type_str += "performance";
        }
        if (type_str.empty()) {
            type_str = "unknown";
        }
        const std::string message_id_name{ data && data->pMessageIdName ? data->pMessageIdName : "" };
        const std::string message{ data && data->pMessage ? data->pMessage : "" };
        log_file(
            "debug_message:\n"
            "    severity: " + severity_str + " (code: " + std::to_string(static_cast<std::uint32_t>(severity)) + ")\n"
            "    type: " + type_str + " (code: " + std::to_string(static_cast<std::uint32_t>(type)) + ")\n"
            "    message_id: " + message_id_name + " (code: " + std::to_string(data ? data->messageIdNumber : 0) + ")\n"
            "    message: " + message + "\n"
        );
        if (!data) {
            return vk::False;
        }
        for (std::uint32_t i{}; i < data->objectCount; ++i) {
            const auto& object{ data->pObjects[i] };
            log_file(
                "    object:\n"
                "        type code: " + std::to_string(static_cast<std::uint32_t>(object.objectType)) + "\n"
                "        handle: " + std::to_string(object.objectHandle) + "\n"
                "        name: " + (object.pObjectName ? std::string{ object.pObjectName } : std::string{}) + "\n"
            );
        }
        return vk::False;
    }

    auto engine_t::transition_image_layout
    (
        std::uint32_t image_index,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        vk::AccessFlags2 src_access_mask,
        vk::AccessFlags2 dest_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dest_stage_mask
    )
    -> void
    {
        vk::ImageMemoryBarrier2 barrier
        {
            .srcStageMask{ src_stage_mask },
            .srcAccessMask{ src_access_mask },
            .dstStageMask{ dest_stage_mask },
            .dstAccessMask{ dest_access_mask },
            .oldLayout{ old_layout },
            .newLayout{ new_layout },
            .srcQueueFamilyIndex{ vk::QueueFamilyIgnored },
            .dstQueueFamilyIndex{ vk::QueueFamilyIgnored },
            .image{ m_swap_chain_image[image_index] },
            .subresourceRange
            {
                .aspectMask{ vk::ImageAspectFlagBits::eColor },
                .baseMipLevel{ 0 },
                .levelCount{ 1 },
                .baseArrayLayer{ 0 },
                .layerCount{ 1 }
            }
        };
        vk::DependencyInfo dependency_info
        {
            .dependencyFlags{},
            .imageMemoryBarrierCount{ 1 },
            .pImageMemoryBarriers{ &barrier }
        };
        m_command_buffer.pipelineBarrier2(dependency_info);
        return;
    }

    engine_t::engine_t
    (
        size_2D window_size,
        std::string name
    ):
    m_window_size{ window_size }
    {
        //Create a window.
        if (!glfwInit()) {
            throw std::runtime_error{ "GLFW initialization failed" };
        }
        glfwWindowHint(glfw_client_api, glfw_no_api);
        glfwWindowHint(glfw_resizable, glfw_false);
        m_window = glfwCreateWindow
        (
            static_cast<int>(m_window_size.x),
            static_cast<int>(m_window_size.y),
            name.c_str(),
            nullptr,
            nullptr
        );
        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error{ "GLFW window creation failed" };
        }
        glfwSetWindowUserPointer(m_window, this);
        glfwSetCursorPosCallback(m_window, [] (GLFWwindow* window, double x, double y) {
            auto* engine{ static_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
            call_callback([] (auto* callback, pos_2D pos) { (*callback)(pos); }, engine->m_mouse_move,
                pos_2D{ static_cast<std::int64_t>(x), static_cast<std::int64_t>(y) });
        });
        glfwSetCursorEnterCallback(m_window, [] (GLFWwindow* window, int entered) {
            if (!entered) {
                auto* engine{ static_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
                call_callback([] (auto* callback) { (*callback)(); }, engine->m_mouse_leave);
            }
        });
        glfwSetMouseButtonCallback(m_window, [] (GLFWwindow* window, int button, int action, int) {
            if (button != glfw_mouse_button_left) return;
            auto* engine{ static_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
            const auto pos{ engine->get_cursor_pos() };
            if (action == glfw_press) {
                call_callback([] (auto* callback, pos_2D value) { (*callback)(value); }, engine->m_mouse_left_click, pos);
            }
            else if (action == glfw_release) {
                call_callback([] (auto* callback, pos_2D value) { (*callback)(value); }, engine->m_mouse_left_release, pos);
            }
        });
        glfwSetScrollCallback(m_window, [] (GLFWwindow* window, double, double y) {
            auto* engine{ static_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
            call_callback([] (auto* callback, pos_2D pos, size_1D amount) { (*callback)(pos, amount); },
                engine->m_mouse_scroll, engine->get_cursor_pos(), size_1D{ static_cast<std::int64_t>(y) });
        });
        glfwSetKeyCallback(m_window, [] (GLFWwindow* window, int key, int, int action, int) {
            if (action == glfw_press || action == glfw_repeat) {
                auto* engine{ static_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
                call_callback([] (auto* callback, std::uint16_t value) { (*callback)(value); },
                    engine->m_key_down, static_cast<std::uint16_t>(key));
            }
        });
        glfwSetCharCallback(m_window, [] (GLFWwindow* window, unsigned int codepoint) {
            if (codepoint <= std::numeric_limits<wchar_t>::max()) {
                auto* engine{ static_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
                call_callback([] (auto* callback, wchar_t value) { (*callback)(value); },
                    engine->m_charw, static_cast<wchar_t>(codepoint));
            }
        });

        //Create an instance.
        vk::ApplicationInfo app_info
        {
            .pApplicationName{ name.c_str() },
            .applicationVersion{ Vulkan_version_encode(0, 0, 5, 0) },
            .pEngineName{ "lgo" },
            .engineVersion{ Vulkan_version_encode(0, 0, 5, 0) },
            .apiVersion{ Vulkan_version_encode(0, 1, 3, 0) }
        };
        std::vector<const char*> layer{};
#ifndef NDEBUG
        const auto available_layers{ m_context.enumerateInstanceLayerProperties() };
        if (std::ranges::any_of(available_layers, [] (const vk::LayerProperties& candidate) {
            return std::string_view{ candidate.layerName.data() } == "VK_LAYER_KHRONOS_validation";
        })) {
            layer.push_back("VK_LAYER_KHRONOS_validation");
        }
#endif
        std::uint32_t glfw_extension_count{};
        const char** glfw_extension{ glfwGetRequiredInstanceExtensions(&glfw_extension_count) };
        std::vector<const char*> instance_extensions( glfw_extension, glfw_extension + glfw_extension_count );
        const auto available_instance_extensions{ m_context.enumerateInstanceExtensionProperties() };
        const bool has_debug_utils{ std::ranges::any_of(available_instance_extensions, [] (const vk::ExtensionProperties& candidate) {
            return std::string_view{ candidate.extensionName.data() } == vk::EXTDebugUtilsExtensionName;
        }) };
        if (has_debug_utils) {
            instance_extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }
        vk::InstanceCreateInfo instance_info
        {
            .pApplicationInfo{ &app_info },
            .enabledLayerCount{ static_cast<std::uint32_t>(layer.size()) },
            .ppEnabledLayerNames{ layer.data() },
            .enabledExtensionCount{ static_cast<std::uint32_t>(instance_extensions.size()) },
            .ppEnabledExtensionNames{ instance_extensions.data() }
        };
        m_instance = vk::raii::Instance{ m_context, instance_info };

        //Create a debug messenger.
        vk::DebugUtilsMessengerCreateInfoEXT messenger_info
        {
            .messageSeverity
            {
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            },
            .messageType
            {
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            },
            .pfnUserCallback{ &debug_callback },
            .pUserData{ this }
        };
        if (has_debug_utils) {
            m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(messenger_info);
        }

        //Create a surface.
        //The window surface needs to be created right after the instance creation,
        //because it can actually influence the physical device selection.
        const vk::SurfaceKHR surface{ glfwCreateVulkanWindowSurface(*m_instance, m_window) };
        if (!surface) {
            throw std::runtime_error{ "Vulkan surface creation failed" };
        }
        m_surface = vk::raii::SurfaceKHR{ m_instance, surface };

        //Create a physical device.
        std::vector<vk::raii::PhysicalDevice> device_available{ m_instance.enumeratePhysicalDevices() };
        std::multimap<int, std::size_t> device_score{};
        std::vector<std::string> device_extensions{ vk::KHRSwapchainExtensionName };
        for (std::size_t device_index{}; device_index < device_available.size(); ++device_index)
        {
            auto& device{ device_available[device_index] };
            const auto device_property{ device.getProperties() };
            const auto device_queue_family{ device.getQueueFamilyProperties() };
            const auto device_extension{ device.enumerateDeviceExtensionProperties() };
            auto device_feature
            {
                device.getFeatures2
                <
                    vk::PhysicalDeviceFeatures2,
                    vk::PhysicalDeviceVulkan11Features,
                    vk::PhysicalDeviceVulkan13Features
                >()
            };
            if
            (
                device_property.apiVersion < Vulkan_version_encode(0, 1, 3, 0) ||
                std::ranges::none_of(std::views::iota(std::size_t{}, device_queue_family.size()), [&] (std::size_t index) {
                    return static_cast<bool>(device_queue_family[index].queueFlags & vk::QueueFlagBits::eGraphics)
                        && device.getSurfaceSupportKHR(static_cast<std::uint32_t>(index), *m_surface);
                }) ||
                std::ranges::any_of
                (
                    device_extensions,
                    [&device_extension] (const std::string& extension_required)
                    {
                        return std::ranges::all_of
                        (
                            device_extension,
                            [&extension_required] (const vk::ExtensionProperties& extension_available)
                            {
                                return extension_available.extensionName != extension_required;
                            }
                        );
                    }
                ) ||
                !device_feature.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters ||
                !device_feature.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering
            ) {
                continue;
            }
            int score{ 0 };
            if (device_property.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                score += 1;
            }
            device_score.emplace(score, device_index);
        }
        if (device_score.empty())
        {
            throw std::runtime_error{ "no suitable Vulkan physical device was found" };
        }
        else
        {
            m_device_physical = std::move(device_available[device_score.rbegin()->second]);
        }

        //Create a logical device.
        const auto queue_family_arr{ m_device_physical.getQueueFamilyProperties() };
        std::optional<std::uint32_t> queue_family_index_found{};
        for (std::size_t index{}; index < queue_family_arr.size(); ++index) {
            if (static_cast<bool>(queue_family_arr[index].queueFlags & vk::QueueFlagBits::eGraphics)
            && m_device_physical.getSurfaceSupportKHR(static_cast<std::uint32_t>(index), *m_surface)) {
                queue_family_index_found = static_cast<std::uint32_t>(index);
                break;
            }
        }
        if (!queue_family_index_found) {
            throw std::runtime_error{ "no Vulkan graphics/present queue family was found" };
        }
        float queue_priority{ 1.0 };
        const std::uint32_t queue_family_index{ *queue_family_index_found };
        vk::DeviceQueueCreateInfo device_queue
        {
            .queueFamilyIndex{ queue_family_index },
            .queueCount{ 1 },
            .pQueuePriorities{ &queue_priority }
        };
        vk::StructureChain feature
        {
            vk::PhysicalDeviceFeatures2{},
            vk::PhysicalDeviceVulkan11Features
            {
                .shaderDrawParameters{ true }
            },
            vk::PhysicalDeviceVulkan13Features
            {
                .dynamicRendering{ true }
            }
        };
        std::vector<const char*> extension_cstr_arr
        {
            std::from_range_t{},
            device_extensions | std::ranges::views::transform([] (const std::string& str) -> const char*
            {
                return str.c_str();
            })
        };
        vk::DeviceCreateInfo device_info
        {
            .pNext{ &feature.get<vk::PhysicalDeviceFeatures2>() },
            .queueCreateInfoCount{ 1 },
            .pQueueCreateInfos{ &device_queue },
            .enabledExtensionCount{ static_cast<std::uint32_t>(extension_cstr_arr.size()) },
            .ppEnabledExtensionNames{ extension_cstr_arr.data() }
        };
        m_device = vk::raii::Device{ m_device_physical, device_info };

        //Create a queue.
        m_command_queue = vk::raii::Queue{ m_device, queue_family_index, 0 };

        //Create a swapchain.
        std::vector<vk::SurfaceFormatKHR> surface_format_arr{ m_device_physical.getSurfaceFormatsKHR(*m_surface) };
        if (surface_format_arr.empty()) {
            throw std::runtime_error{ "the selected Vulkan device has no surface formats" };
        }
        auto surface_format_it
        {
            std::ranges::find_if
            (
                surface_format_arr,
                [] (vk::SurfaceFormatKHR format) -> bool
                {
                    return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
                }
            )
        };
        vk::SurfaceFormatKHR surface_format{ surface_format_it == surface_format_arr.end() ? surface_format_arr.front() : *surface_format_it };
        std::vector<vk::PresentModeKHR> surface_present_mode_arr{ m_device_physical.getSurfacePresentModesKHR(*m_surface) };
        //The vk::PresentModeKHR::eFifo mode is guaranteed to be available.
        vk::PresentModeKHR surface_present_mode{ vk::PresentModeKHR::eFifo };
        vk::SurfaceCapabilitiesKHR surface_capability{ m_device_physical.getSurfaceCapabilitiesKHR(*m_surface) };
        vk::Extent2D extent{};
        if (surface_capability.currentExtent.width == std::numeric_limits<std::uint32_t>::max())
        {
            int width{}, height{};
            glfwGetFramebufferSize(m_window, &width, &height);
            extent.width = std::clamp
            (
                static_cast<std::uint32_t>(width),
                surface_capability.minImageExtent.width,
                surface_capability.maxImageExtent.width
            );
            extent.height = std::clamp
            (
                static_cast<std::uint32_t>(height),
                surface_capability.minImageExtent.height,
                surface_capability.maxImageExtent.height
            );
        }
        else
        {
            extent = surface_capability.currentExtent;
        }
        m_swap_chain_extent = extent;
        std::uint32_t image_count_min{ surface_capability.minImageCount + 1 };
        if (surface_capability.maxImageCount && (image_count_min > surface_capability.maxImageCount))
        {
            image_count_min = surface_capability.maxImageCount;
        }
        vk::SwapchainCreateInfoKHR swapchain_info
        {
            .surface{ *m_surface },
            .minImageCount{ image_count_min },
            .imageFormat{ surface_format.format },
            .imageColorSpace{ surface_format.colorSpace },
            .imageExtent{ extent },
            .imageArrayLayers{ 1 },
            .imageUsage{ vk::ImageUsageFlagBits::eColorAttachment },
            .imageSharingMode{ vk::SharingMode::eExclusive },
            .preTransform{ surface_capability.currentTransform },
            .compositeAlpha{ vk::CompositeAlphaFlagBitsKHR::eOpaque },
            .presentMode{ surface_present_mode },
            .clipped{ true },
            .oldSwapchain{ nullptr }
        };
        m_swap_chain = vk::raii::SwapchainKHR{ m_device, swapchain_info };
        m_swap_chain_image = m_swap_chain.getImages();

        //Create image views.
        vk::ImageViewCreateInfo image_view_info
        {
            .viewType{ vk::ImageViewType::e2D },
            .format{ surface_format.format },
            .subresourceRange
            {
                .aspectMask{ vk::ImageAspectFlagBits::eColor },
                .baseMipLevel{ 0 },
                .levelCount{ 1 },
                .baseArrayLayer{ 0 },
                .layerCount{ 1 }
            }
        };
        for (auto& image : m_swap_chain_image)
        {
            image_view_info.image = image;
            m_swap_chain_image_view.emplace_back(m_device, image_view_info);
        }

        //Create shader module.
        const std::vector<unsigned char> shader_bytes{ get_file(LGO_SHADER_PATH) };
        if (shader_bytes.empty() || shader_bytes.size() % sizeof(std::uint32_t) != 0) {
            throw std::runtime_error{ "the compiled SPIR-V shader is empty or malformed" };
        }
        std::vector<std::uint32_t> shader_code(shader_bytes.size() / sizeof(std::uint32_t));
        std::memcpy(shader_code.data(), shader_bytes.data(), shader_bytes.size());
        vk::ShaderModuleCreateInfo shader_module_info
        {
            .codeSize{ shader_bytes.size() },
            .pCode{ shader_code.data() }
        };
        vk::raii::ShaderModule shader_module{ m_device, shader_module_info };

        //Create graphics pipeline.
        const std::array shader_stage
        {
            vk::PipelineShaderStageCreateInfo
            {
                .stage{ vk::ShaderStageFlagBits::eVertex },
                .module{ *shader_module },
                .pName{ "vertex_main" }
            },
            vk::PipelineShaderStageCreateInfo
            {
                .stage{ vk::ShaderStageFlagBits::eFragment },
                .module{ *shader_module },
                .pName{ "fragment_main" }
            }
        };
        std::vector<vk::DynamicState> dynamic_state_arr
        {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        vk::PipelineDynamicStateCreateInfo dynamic_state_info
        {
            .dynamicStateCount{ static_cast<std::uint32_t>(dynamic_state_arr.size()) },
            .pDynamicStates{ dynamic_state_arr.data()  }
        };
        vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
        vk::PipelineInputAssemblyStateCreateInfo input_assembly
        {
            .topology{ vk::PrimitiveTopology::eTriangleList }
        };
        vk::Viewport viewport
        {
            .x{ 0.0f },
            .y{ 0.0f },
            .width{ static_cast<float>(m_swap_chain_extent.width) },
            .height{ static_cast<float>(m_swap_chain_extent.height) },
            .minDepth{ 0.0f },
            .maxDepth{ 1.0f }
        };
        vk::Rect2D scissor
        {
            .offset{ 0, 0 },
            .extent{ extent }
        };
        vk::PipelineViewportStateCreateInfo viewport_state
        {
            .viewportCount{ 1 },
            .scissorCount{ 1 }
        };
        vk::PipelineRasterizationStateCreateInfo rasterization
        {
            .depthClampEnable{ vk::False },
            .rasterizerDiscardEnable{ vk::False },
            .polygonMode{ vk::PolygonMode::eFill },
            .cullMode{ vk::CullModeFlagBits::eBack },
            .frontFace{ vk::FrontFace::eCounterClockwise },
            .depthBiasEnable{ vk::False },
            .lineWidth{ 1.0f }
        };
        vk::PipelineMultisampleStateCreateInfo multisample
        {
            .rasterizationSamples{ vk::SampleCountFlagBits::e1 },
            .sampleShadingEnable{ vk::False }
        };
        vk::PipelineColorBlendAttachmentState color_blend_arr
        {
            .blendEnable{ vk::False },
            .colorWriteMask
            {
                vk::ColorComponentFlagBits::eR |
                vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eA
            }
        };
        vk::PipelineColorBlendStateCreateInfo color_blend
        {
            .logicOpEnable{ vk::False },
            .attachmentCount{ 1 },
            .pAttachments{ &color_blend_arr }
        };
        vk::PipelineLayoutCreateInfo pipeline_layout_info
        {
            .setLayoutCount{ 0 },
            .pushConstantRangeCount{ 0 }
        };
        m_pipeline_layout = vk::raii::PipelineLayout{ m_device, pipeline_layout_info };
        vk::StructureChain
        <
            vk::GraphicsPipelineCreateInfo,
            vk::PipelineRenderingCreateInfo
        >
        pipeline_info
        {
            {
                .stageCount{ static_cast<std::uint32_t>(shader_stage.size()) },
                .pStages{ shader_stage.data() },
                .pVertexInputState{ &vertex_input_info },
                .pInputAssemblyState{ &input_assembly },
                .pViewportState{ &viewport_state },
                .pRasterizationState{ &rasterization },
                .pMultisampleState{ &multisample },
                .pColorBlendState{ &color_blend },
                .pDynamicState{ &dynamic_state_info },
                .layout{ *m_pipeline_layout },
                .renderPass{ nullptr }
            },
            {
                .colorAttachmentCount{ 1 },
                .pColorAttachmentFormats{ &surface_format.format }
            }
        };
        m_pipeline = vk::raii::Pipeline{ m_device, nullptr, pipeline_info.get<vk::GraphicsPipelineCreateInfo>() };

        //Create a command pool.
        vk::CommandPoolCreateInfo command_pool_info
        {
            .flags{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer },
            .queueFamilyIndex{ queue_family_index }
        };
        m_command_pool = vk::raii::CommandPool{ m_device, command_pool_info };

        //Create a command buffer.
        vk::CommandBufferAllocateInfo command_buffer_info
        {
            .commandPool{ *m_command_pool },
            .level{ vk::CommandBufferLevel::ePrimary },
            .commandBufferCount{ 1 }
        };
        m_command_buffer = std::move(vk::raii::CommandBuffers{ m_device, command_buffer_info }.front());

        //Create semaphores.
        m_semaphore_image = vk::raii::Semaphore{ m_device, vk::SemaphoreCreateInfo{} };
        m_semaphore_draw = vk::raii::Semaphore{ m_device, vk::SemaphoreCreateInfo{} };

        //Create a fence.
        m_fence = vk::raii::Fence
        {
            m_device,
            {
                .flags{ vk::FenceCreateFlagBits::eSignaled }
            }
        };

        return;
    }

    engine_t::~engine_t() {
        if (*m_device) {
            m_device.waitIdle();
        }
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    auto engine_t::flush() -> void {
        m_device.waitIdle();
    }

    auto engine_t::redraw() -> void {
        glfwPostEmptyEvent();
    }

    auto engine_t::message_loop() -> bool {
        glfwPollEvents();

        //Acquire next image.
		if (m_device.waitForFences(*m_fence, vk::True, std::numeric_limits<std::uint64_t>::max()) != vk::Result::eSuccess)
		{
			throw std::runtime_error{ "waiting for the Vulkan draw fence failed" };
		}
        auto [result, image_index]
        {
            m_swap_chain.acquireNextImage
            (
                std::numeric_limits<std::uint64_t>::max(),
                *m_semaphore_image,
                nullptr
            )
        };
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error{ "acquiring the next Vulkan swap-chain image failed" };
        }

        //Record the command buffer.
        m_command_buffer.reset();
        m_command_buffer.begin({});
        transition_image_layout
        (
            image_index,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput
        );
        vk::ClearValue clear_color{ vk::ClearColorValue{ std::array{ 0.0f, 0.0f, 0.0f, 1.0f } } };
        vk::RenderingAttachmentInfo attachment_info
        {
            .imageView{ *m_swap_chain_image_view[image_index] },
            .imageLayout{ vk::ImageLayout::eColorAttachmentOptimal },
            .loadOp{ vk::AttachmentLoadOp::eClear },
            .storeOp{ vk::AttachmentStoreOp::eStore },
            .clearValue{ clear_color }
        };
        vk::RenderingInfo render_info
        {
            .renderArea
            {
                .offset{ 0, 0 },
                .extent{ m_swap_chain_extent }
            },
            .layerCount{ 1 },
            .colorAttachmentCount{ 1 },
            .pColorAttachments{ &attachment_info }
        };
        m_command_buffer.beginRendering(render_info);
        m_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
        m_command_buffer.setViewport
        (
            0,
            vk::Viewport
            {
                0.0f,
                0.0f,
                static_cast<float>(m_swap_chain_extent.width),
                static_cast<float>(m_swap_chain_extent.height),
                0.0f,
                1.0f
            }
        );
        m_command_buffer.setScissor
        (
            0,
            vk::Rect2D
            {
                vk::Offset2D(0, 0),
                m_swap_chain_extent
            }
        );
        m_command_buffer.draw(3, 1, 0, 0);
        m_command_buffer.endRendering();
        transition_image_layout
        (
            image_index,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe
        );
        m_command_buffer.end();
        //Submit the command buffer.
        vk::PipelineStageFlags stage_wait{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
        const vk::SubmitInfo submit_info
        {
            .waitSemaphoreCount{ 1 },
            .pWaitSemaphores{ &*m_semaphore_image },
            .pWaitDstStageMask{ &stage_wait },
            .commandBufferCount{ 1 },
            .pCommandBuffers{ &*m_command_buffer },
            .signalSemaphoreCount{ 1 },
            .pSignalSemaphores{ &*m_semaphore_draw }
        };
        // Only make the fence unsignalled once there is work ready to signal it.
        // Resetting before image acquisition can leave the next frame blocked
        // forever when acquireNextImage fails.
        m_device.resetFences(*m_fence);
        m_command_queue.submit(submit_info, *m_fence);

        //Present the image.
        const vk::PresentInfoKHR present_info
        {
            .waitSemaphoreCount{ 1 },
            .pWaitSemaphores{ &*m_semaphore_draw },
            .swapchainCount{ 1 },
            .pSwapchains{ &*m_swap_chain },
            .pImageIndices{ &image_index }
        };
        result = m_command_queue.presentKHR(present_info);
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error{ "presenting the Vulkan swap-chain image failed" };
        }

        return m_exit || glfwWindowShouldClose(m_window);
    }

    auto engine_t::log_info_queue() -> void {
        // Vulkan validation output is handled synchronously by debug_callback.
    }

    auto engine_t::set_exit() -> void {
        m_exit = true;
        glfwSetWindowShouldClose(m_window, 1);
        glfwPostEmptyEvent();
    }

    auto engine_t::get_exit() -> bool {
        return m_exit;
    }

    auto engine_t::get_window_pos() -> pos_2D {
        int x{}, y{};
        glfwGetWindowPos(m_window, &x, &y);
        return pos_2D{ x, y };
    }

    auto engine_t::get_window_size() -> size_2D {
        int width{}, height{};
        glfwGetFramebufferSize(m_window, &width, &height);
        return size_2D{ width, height };
    }

    auto engine_t::get_cursor_pos() -> pos_2D {
        double x{}, y{};
        glfwGetCursorPos(m_window, &x, &y);
        return pos_2D{ static_cast<std::int64_t>(x), static_cast<std::int64_t>(y) };
    }

    auto engine_t::device_get() const -> const vk::raii::Device& {
        return m_device;
    }

    auto engine_t::command_queue_get() -> vk::raii::Queue& {
        return m_command_queue;
    }

    auto engine_t::add_mouse_move(std::function<void(pos_2D)> callback) -> std::function<void(pos_2D)>* {
        std::function<void(pos_2D)>* out{ new std::function<void(pos_2D)>{ callback } };
        if (m_mouse_move.m_calling_callback) {
            m_mouse_move.m_pending.emplace(out);
        }
        else {
            m_mouse_move.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::remove_mouse_move(std::function<void(pos_2D)>* in) -> void {
        remove_callback(m_mouse_move, in);
    }

    auto engine_t::add_mouse_leave(std::function<void(void)> callback) -> std::function<void(void)>* {
        std::function<void(void)>* out{ new std::function<void(void)>{ callback } };
        if (m_mouse_leave.m_calling_callback) {
            m_mouse_leave.m_pending.emplace(out);
        }
        else {
            m_mouse_leave.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::remove_mouse_leave(std::function<void(void)>* in) -> void {
        remove_callback(m_mouse_leave, in);
    }

    auto engine_t::add_mouse_left_click(std::function<void(pos_2D)> callback) -> std::function<void(pos_2D)>* {
        std::function<void(pos_2D)>* out{ new std::function<void(pos_2D)>{ callback } };
        if (m_mouse_left_click.m_calling_callback) {
            m_mouse_left_click.m_pending.emplace(out);
        }
        else {
            m_mouse_left_click.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::remove_mouse_left_click(std::function<void(pos_2D)>* in) -> void {
        remove_callback(m_mouse_left_click, in);
    }

    auto engine_t::add_mouse_left_release(std::function<void(pos_2D)> callback) -> std::function<void(pos_2D)>* {
        std::function<void(pos_2D)>* out{ new std::function<void(pos_2D)>{ callback } };
        if (m_mouse_left_release.m_calling_callback) {
            m_mouse_left_release.m_pending.emplace(out);
        }
        else {
            m_mouse_left_release.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::remove_mouse_left_release(std::function<void(pos_2D)>* in) -> void {
        remove_callback(m_mouse_left_release, in);
    }

    auto engine_t::add_mouse_scroll(std::function<void(pos_2D, size_1D)> callback) -> std::function<void(pos_2D, size_1D)>* {
        std::function<void(pos_2D, size_1D)>* out{ new std::function<void(pos_2D, size_1D)>{ callback } };
        if (m_mouse_scroll.m_calling_callback) {
            m_mouse_scroll.m_pending.emplace(out);
        }
        else {
            m_mouse_scroll.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::remove_mouse_scroll(std::function<void(pos_2D, size_1D)>* in) -> void {
        remove_callback(m_mouse_scroll, in);
    }

    auto engine_t::add_key_down(std::function<void(std::uint16_t)> callback) -> std::function<void(std::uint16_t)>* {
        std::function<void(std::uint16_t)>* out{ new std::function<void(std::uint16_t)>{ callback } };
        if (m_key_down.m_calling_callback) {
            m_key_down.m_pending.emplace(out);
        }
        else {
            m_key_down.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::remove_key_down(std::function<void(std::uint16_t)>* in) -> void {
        remove_callback(m_key_down, in);
    }

    auto engine_t::add_charw(std::function<void(wchar_t)> callback) -> std::function<void(wchar_t)>* {
        std::function<void(wchar_t)>* out{ new std::function<void(wchar_t)>{ callback } };
        if (m_charw.m_calling_callback) {
            m_charw.m_pending.emplace(out);
        }
        else {
            m_charw.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::remove_charw(std::function<void(wchar_t)>* in) -> void {
        remove_callback(m_charw, in);
    }
}
