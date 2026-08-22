module lgo.GUI.engine;

import std;

import external.Vulkan;
import external.GLFW;

namespace lgo {
    auto engine_t::track_mouse_event() -> void {
        TRACKMOUSEEVENT stu{};
        stu.cbSize = sizeof stu;
        stu.dwFlags = TME_LEAVE;
        stu.hwndTrack = m_window;
        TrackMouseEvent(&stu);
        return;
    }

    template<typename t_callback_set, typename t_caller, typename... t_in>
    auto engine_t::call_callback(const t_caller& caller, t_callback_set& callback, t_in... in) -> void {
        callback.m_calling_callback = true;
        static std::uint64_t depth{ 0 };
        ++depth;
        typename decltype(callback.m_effective)::key_type current{};
        for (auto i{ callback.m_effective.begin() }; i != callback.m_effective.end(); i = callback.m_effective.upper_bound(current)) {
            current = *i;
            caller(current, in...);
        }
        --depth;
        if (!depth) {
            callback.m_effective.insert(callback.m_pending.begin(), callback.m_pending.end());
            callback.m_pending.clear();
        }
        callback.m_calling_callback = false;
        return;
    }

    VKAPI_ATTR auto VKAPI_CALL engine_t::debug_callback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagBitsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* data,
        void* engine_voidptr
    ) -> VkBool32 {
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
        switch (type) {
        case vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral: {
            severity_str = "general";
            break;
        }
        case vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation: {
            severity_str = "validation";
            break;
        }
        case vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance: {
            severity_str = "performance";
            break;
        }
        default: {
            severity_str = "unknown";
            break;
        }
        }
        log_file(
            "debug_message:\n"
            "    severity: " + severity_str + " (code: " + severity + ")\n"
            "    type: " + type_str + " (code: " + type + ")\n"
            "    message_id: " + data->pMessageIdName + " (code: " + data->messageIdNumber + ")\n"
            "    message: " + data->pMessage + "\n"
        );
        for (int i{ 0 }; i < data->objectCount; ++i) {
            log_file(
                "    object:\n"
                "        type: " "(code: " + data->pOjbects[i]->objectType + ")\n"
                "        handle: " + data->pOjbects[i]->objectHandle + "\n"
                "        name: " + data->pOjbects[i]->pobjectName + "\n"
            );
        }
        if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
            throw;
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
            .dstStageMask{ dst_stage_mask },
            .dstAccessMask{ dst_access_mask },
            .oldLayout{ old_layout },
            .newLayout{ new_layout },
            .srcQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
            .dstQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
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
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_window = glfwCreateWindow
        (
            static_cast<int>(m_window_size.x),
            static_cast<int>(m_window_size.y),
            name.c_str(),
            nullptr,
            nullptr
        );

        //Create an instance.
        vk::ApplicationInfo app_info
        {
            .pApplicationName{ name },
            .applicationVersion{ Vulkan_version_encode(0, 0, 5, 0) },
            .pEngineName{ "lgo" },
            .engineVersion{ Vulkan_version_encode(0, 0, 5, 0) },
            .apiVersion{ Vulkan_version_encode(0, 1, 4, 0) }
        };
        std::vector<const char*> layer
        {
            "VK_LAYER_KHRONOS_validation"
        };
        std::uint32_t glfw_extension_count{};
        const char** glfw_extension{ glfwGetRequiredInstanceExtensions(&glfw_extension_count) };
        std::vector<const char*> extension( glfw_extension, glfw_extension + glfw_extension_count );
        extension.push_back("VK_EXT_debug_utils");
        vk::InstanceCreateInfo instance_info
        {
            .pApplicationInfo{ &app_info },
            .enabledLayerCount{ static_cast<std::uint32_t>(layer.size()) },
            .ppEnabledLayerNames{ layer.data() }
            .enabledExtensionCount{ static_cast<std::uint32_t>(extension.size()) },
            .ppEnabledExtensionNames{ extension.data() }
        };
        m_instance = vk::raii::Instance{ m_context, instance_info };

        //Create a debug messenger.
        vk::DebugUtilsMessengerCreateInfoEXT messenger_info
        {
            .messageSeverity
            {
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose ||
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo ||
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning ||
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            }
            .messageType
            {
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral ||
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation ||
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            }
            .pfnUserCallback{ &debug_callback },
            .pUserData{ this }
        };
        m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(messenger_info);

        //Create a surface.
        //The window surface needs to be created right after the instance creation,
        //because it can actually influence the physical device selection.
        VkSurfaceKHR surface_c{};
        glfwCreateWindowSurface(*m_instance, m_window, nullptr, &surface_c);
        m_surface = vk::raii::SurfaceKHR{ m_instance, surface_c };

        //Create a physical device.
        std::vector<vk::raii::PhysicalDevice> device_available{ m_instance.enumeratePhysicalDevices() };
        std::multimap<int, vk::raii::PhysicalDevice> device_score{};
        std::vector<std::string> extension{ "VK_KHR_swapchain" };
        for (vk::raii::PhysicalDevice device : device_available)
        {
            device_property{ device.getProperties() };
            device_queue_family{ device.getQueueFamilyProperties() };
            device_extension{ device.enumerateDeviceExtensionProperties() };
            device_feature
            {
                device.getFeatures2
                <
                    vk::PhysicalDeviceFeatures2,
                    vk::PhysicalDeviceVulkan11Features,
                    vk::PhysicalDeviceVulkan13Features,
                    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
                >()
            };
            if
            (
                device_property.apiVersion < Vulkan_version_encode(0, 1, 4, 0) ||
                std::ranges::all_of
                (
                    device_queue_family | std::ranges::views::enumerate,
                    [device&] (const std::tuple<>& pair) -> bool
                    {
                        auto [index, queue_family]{ pair };
                        return
                            (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) &&
                            device.getSurfaceSupportKHR(index, *m_surface);
                    }
                ) ||
                std::ranges::any_of
                (
                    extension,
                    [device_extension] (const & extension)
                    {
                        return std::ranges::all_of
                        (
                            device_extension,
                            [extension] (const & device_extension)
                            {
                                return device_extension.extensionName != extension;
                            }
                        );
                    }
                ) ||
                !device_feature.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters ||
                !device_feature.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering ||
                !device_feature.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState
            ) {
                continue;
            }
            int score{ 0 };
            if (device_property.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                score += 1;
            }
            device_score.emplace(score, device);
        }
        if (device_score.empty())
        {
            throw;
        }
        else
        {
            m_device_physical = *device_score.rbegin();
        }

        //Create a logical device.
        std::vector<vk::QueueFamilyProperties2> queue_family_arr{ m_device_physical.getQueueFamilyProperties2() };
        auto queue_family_iter
        {
            std::ranges::find_if
            (
                queue_family_arr,
                [m_device_physical&] (const vk::QueueFamilyProperties2& queue_family) -> bool
                {
                    return
                        (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) &&
                        m_device_physical.getSurfaceSupportKHR(index, *m_surface);
                }
            )
        };
        float queue_priority{ 1.0 };
        std::uint32_t queue_family_index{ std::ranges::distance(queue_family_arr.begin(), queue_family_iter) };
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
            },
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
            {
                .extendedDynamicState{ true }
            }
        };
        std::vector<const char*> extension_cstr_arr
        {
            std::from_range_t{},
            extension | std::ranges::views::transform([] (const std::string& str) -> const char*
            {
                return str.c_str();
            })
        };
        vk::DeviceCreateInfo device_info
        {
            .pNext{ &feature.get<vk::PhysicalDeviceFeatures2>() },
            .queueCreateInfoCount{ 1 },
            .ppQueueCreateInfos{ &device_queue },
            .enabledExtensionCount{ extension_cstr_arr.size() },
            .ppEnabledExtensionNames{ extension_cstr_arr.data() }
        };
        m_device = vk::raii::Device{ m_device_physical, device_info };

        //Create a queue.
        m_queue = vk::raii::Queue{ device, queue_family_index, 0 };

        //Create a swapchain.
        std::vector<vk::SurfaceFormatKHR> surface_format_arr{ m_device_physical.getSurfaceFormats2KHR(*m_surface) };
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
        vk::SurfaceFormatKHR surface_format{ *surface_format_it };
        std::vector<vk::PresentModeKHR> surface_present_mode_arr{ m_device_physical.getSurfacePresentModesKHR(*m_surface) };
        //The vk::PresentModeKHR::eFifo mode is guaranteed to be available.
        vk::PresentModeKHR surface_present_mode{ vk::PresentModeKHR::eFifo };
        vk::SurfaceCapabilitiesKHR surface_capability{ m_device_physical.getSurfaceCapabilities2KHR(*m_surface) };
        vk::Extent2D extent{};
        if (surface_capability.currentExtent.width == std::numeric_limits<std::uint32_t>::max())
        {
            int width{}, height{};
            glfwGetFramebufferSize(m_window, &width, &height);
            extent.width = std::clamp
            (
                width,
                surface_capability.minImageExtent.width,
                surface_capability.maxImageExtent.width
            );
            extent.height = std::clamp
            (
                height,
                surface_capability.minImageExtent.height,
                surface_capability.maxImageExtent.height
            );
        }
        else
        {
            extent = surface_capability.currentExtent;
        }
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
        std::vector<unsigned char> shader_code{ get_file("slang.spv") };
        //The default allocator of std::vector already ensures that the data satisfies the alignment requirements of uint32_t.
        vk::ShaderModuleCreateInfo shader_module_info
        {
            .codeSize{ shader_code.size() },
            .pCode{ reinterpret_cast<const uint32_t*>(shader_code.data()) }
        };
        vk::raii::ShaderModule shader_module{ m_device, shader_module_info };

        //Create graphics pipeline.
        auto shader_stage
        {
            std::make_array<vk::PipelineShaderStageCreateInfo>(
            {
                .stage{ vk::ShaderStageFlagBits::eVertex },
                .module{ shader_module },
                .pName{ "vertex_main" }
            },
            {
                .stage{ vk::ShaderStageFlagBits::eFragment },
                .module{ shader_module },
                .pName{ "fragment_main" }
            })
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
            .width{ static_cast<float>(swapChainExtent.width) },
            .height{ static_cast<float>(swapChainExtent.height) },
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
            .frontFace{ vk::FrontFace::eCounterclockwise },
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
                .stageCount{ shader_stage.size() },
                .pStages{ shader_stage.data() },
                .pVertexInputState{ &vertexInputInfo },
                .pInputAssemblyState{ &inputAssembly },
                .pViewportState{ &viewportState },
                .pRasterizationState{ &rasterizer },
                .pMultisampleState{ &multisampling },
                .pColorBlendState{ &colorBlending },
                .pDynamicState{ &dynamicState },
                .layout{ m_pipeline_layout },
                .renderPass{ nullptr }
            },
            {
                .colorAttachmentCount{ 1 },
                .pColorAttachmentFormats{ &swapChainSurfaceFormat.format }
            }
        };
        m_pipeline = vk::raii::Pipeline{ m_device, nullptr, pipeline_info.get<vk::GraphicsPipelineCreateInfo>() };

        //Create a command pool.
        vk::CommandPoolCreateInfo command_pool_info
        {
            .flags{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer },
            .queueFamilyIndex = queue_family_index
        };
        m_command_pool = vk::raii::CommandPool{ m_device, command_pool_info };

        //Create a command buffer.
        vk::CommandBufferAllocateInfo command_buffer_info
        {
            .commandPool{ m_command_pool },
            .level{ vk::CommandBufferLevel::ePrimary },
            .commandBufferCount{ 1 }
        };
        m_command_buffer = std::move(vk::raii::CommandBuffers{ device, allocInfo }.front());

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
        m_device.waitIdle();
        rect_primitive_t::uninit();
        glfwDestroyWindow(m_window);
        glfwTerminate();
        log_info_queue();
        return;
    }

    auto engine_t::flush() -> void {
        m_command_queue.flush();
        return;
    }

    auto engine_t::redraw() -> void {
        InvalidateRect(m_window, nullptr, false);
        return;
    }

    auto engine_t::message_loop() -> bool {
        glfwPollEvents();

        //Acquire next image.
		if (m_device.waitForFences(*drawFence, vk::True, std::numeric_limits<std::uint64_t>::max()) != vk::Result::eSuccess)
		{
			throw;
		}
		device.resetFences(*drawFence);
        auto [result, image_index]
        {
            m_swap_chain.acquireNextImage
            (
                std::numeric_limits<std::uint64_t>::max(),
                *m_semaphore_image,
                VK_NULL_HANDLE
            )
        };

        //Record the command buffer.
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
        vk::ClearValue clear_color{ vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f) };
        vk::RenderingAttachmentInfo attachment_info
        {
            .imageView{ m_swap_chain_image_view[image_index] },
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
        /*std::unique_lock rect_lock{ ptr->m_rect_mutex };
        rect_primitive_t::render(ptr->m_rect_primitive, command_list);
        rect_lock.unlock();*/

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
        m_queue.submit(submit_info, *m_fence);

        //Present the image.
        const vk::PresentInfoKHR present_info
        {
            .waitSemaphoreCount{ 1 },
            .pWaitSemaphores{ &*m_semaphore_draw },
            .swapchainCount{ 1 },
            .pSwapchains{ &*m_swap_chain },
            .pImageIndices{ &image_index }
        };
        result = m_queue.presentKHR(present_info);

        return glfwWindowShouldClose(m_window);
    }

    auto engine_t::log_info_queue() -> void {
        ::log_info_queue(m_info_queue);
        return;
    }

    auto engine_t::set_exit() -> void {
        m_exit = true;
        return;
    }

    auto engine_t::get_exit() -> bool {
        return m_exit;
    }

    auto engine_t::get_window_pos() -> pos_2D {
        return m_window_pos;
    }

    auto engine_t::get_window_size() -> size_2D {
        return m_window_size;
    }

    auto engine_t::get_cursor_pos() -> pos_2D {
        POINT cursor_pos{};
        GetCursorPos(&cursor_pos);
        ScreenToClient(m_window, &cursor_pos);
        return pos_2D{ cursor_pos.x, cursor_pos.y };
    }

    auto engine_t::device_get() const -> Microsoft::WRL::ComPtr<ID3D12Device2> {
        return m_device;
    }

    auto engine_t::command_queue_get() -> command_queue_t& {
        return m_command_queue;
    }

    auto engine_t::add_rect(pos_2D pos, size_2D size, float depth, color_t color, std::string name) -> rect_primitive_t* {
        rect_primitive_t* out{ new rect_primitive_t{ this, pos, size, depth, color, name } };
        std::unique_lock lock{ m_rect_mutex };
        m_rect_primitive.emplace(out);
        redraw();
        return out;
    }

    auto engine_t::add_rect(pos_2D pos, size_2D size, float depth, pos_2D clip_pos, size_2D clip_size
    , const SRV_t& SRV, pos_2D texture_pos, size_2D texture_axis_x, size_2D texture_axis_y, std::string name) -> rect_primitive_t* {
        rect_primitive_t* out{ new rect_primitive_t{ this, pos, size, depth
        , clip_pos, clip_size, SRV, texture_pos, texture_axis_x, texture_axis_y, name } };
        std::unique_lock lock{ m_rect_mutex };
        m_rect_primitive.emplace(out);
        redraw();
        return out;
    }

    auto engine_t::remove_rect(rect_primitive_t* in) -> void {
        std::unique_lock lock{ m_rect_mutex };
        m_rect_primitive.erase(in);
        flush();
        delete in;
        redraw();
        return;
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
        if (!m_mouse_move.m_effective.erase(in)) {
            m_mouse_move.m_pending.erase(in);
        }
        delete in;
        return;
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
        if (!m_mouse_leave.m_effective.erase(in)) {
            m_mouse_leave.m_pending.erase(in);
        }
        delete in;
        return;
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
        if (!m_mouse_left_click.m_effective.erase(in)) {
            m_mouse_left_click.m_pending.erase(in);
        }
        delete in;
        return;
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
        if (!m_mouse_left_release.m_effective.erase(in)) {
            m_mouse_left_release.m_pending.erase(in);
        }
        delete in;
        return;
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
        if (!m_mouse_scroll.m_effective.erase(in)) {
            m_mouse_scroll.m_pending.erase(in);
        }
        delete in;
        return;
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
        if (!m_key_down.m_effective.erase(in)) {
            m_key_down.m_pending.erase(in);
        }
        delete in;
        return;
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
        if (!m_charw.m_effective.erase(in)) {
            m_charw.m_pending.erase(in);
        }
        delete in;
        return;
    }
}
