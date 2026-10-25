module;

#include <vulkan/vk_platform.h>
#include <glm/glm.hpp>

module lgo.GUI.primitive;

import std;

import external.Vulkan;
import external.GLFW;

import lgo.math.vec;
import lgo.io.file;
import lgo.GUI.Vulkan;

namespace lgo {
    template<typename t_callback_set, typename... t_in>
    auto engine_t::call_callback(t_callback_set& callback, t_in... in) -> void
    {
        const bool was_calling{ std::exchange(callback.m_calling_callback, true) };
        auto finish = [&]
        {
            callback.m_calling_callback = was_calling;
            if (!was_calling)
            {
                callback.m_effective.merge(callback.m_pending);
            }
        };
        try
        {
            typename decltype(callback.m_effective)::key_type current{};
            for (auto i{ callback.m_effective.begin() }; i != callback.m_effective.end();
                i = callback.m_effective.upper_bound(current))
            {
                current = *i;
                (*current)(in...);
            }
        }
        catch (...)
        {
            finish();
            throw;
        }
        finish();
        return;
    }

    auto engine_t::GLFW_callback_window_refresh(GLFWwindow* window) -> void
    {
        auto* engine{ static_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
        try
        {
            // Win32's modal move/size loop blocks glfwPollEvents(); repaint before it returns.
            engine->render_frame();
        }
        catch (...)
        {
            // Exceptions must not cross the native window-procedure boundary.
            engine->m_refresh_error = std::current_exception();
        }
    }

    auto engine_t::GLFW_callback_cursor_enter(GLFWwindow* window, int entered) -> void
    {
        auto ptr{ reinterpret_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
        if (ptr->m_refresh_error) return;
        try
        {
            call_callback(ptr->m_callback_mouse_enter, entered);
        }
        catch (...) { ptr->m_refresh_error = std::current_exception(); }
    }
    
    auto engine_t::GLFW_callback_cursor_move(GLFWwindow* window, double x, double y) -> void
    {
        auto ptr{ reinterpret_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
        if (ptr->m_refresh_error) return;
        try
        {
            call_callback
            (
                ptr->m_callback_mouse_move,
                pos_2D{ static_cast<std::int64_t>(x), static_cast<std::int64_t>(y) }
            );
        }
        catch (...) { ptr->m_refresh_error = std::current_exception(); }
    }

    auto engine_t::GLFW_callback_mouse_button(GLFWwindow* window, int button, int action, int mods) -> void
    {
        auto ptr{ reinterpret_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
        if (ptr->m_refresh_error) return;
        try
        {
            call_callback
            (
                ptr->m_callback_mouse_button,
                button,
                action,
                mods
            );
        }
        catch (...) { ptr->m_refresh_error = std::current_exception(); }
    }

    auto engine_t::GLFW_callback_wheel(GLFWwindow* window, double x, double y) -> void
    {
        auto ptr{ reinterpret_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
        if (ptr->m_refresh_error) return;
        try
        {
            call_callback
            (
                ptr->m_callback_mouse_scroll,
                size_2D{ static_cast<std::int64_t>(x), static_cast<std::int64_t>(y) }
            );
        }
        catch (...) { ptr->m_refresh_error = std::current_exception(); }
    }

    auto engine_t::GLFW_callback_key(GLFWwindow* window, int key, int scancode, int action, int mod) -> void
    {
        auto ptr{ reinterpret_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
        if (ptr->m_refresh_error) return;
        try
        {
            call_callback(ptr->m_callback_key, key, action, mod);
        }
        catch (...) { ptr->m_refresh_error = std::current_exception(); }
    }

    auto engine_t::GLFW_callback_char(GLFWwindow* window, unsigned int codepoint) -> void
    {
        auto ptr{ reinterpret_cast<engine_t*>(glfwGetWindowUserPointer(window)) };
        if (ptr->m_refresh_error) return;
        try
        {
            call_callback(ptr->m_callback_cha, static_cast<char32_t>(codepoint));
        }
        catch (...) { ptr->m_refresh_error = std::current_exception(); }
    }

    VKAPI_ATTR auto VKAPI_CALL engine_t::debug_callback
    (
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* data,
        void* engine_voidptr [[maybe_unused]]
    )
    -> vk::Bool32
    {
        std::string severity_str{};
        switch (severity)
        {
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
            {
                severity_str = "verbose";
                break;
            }
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            {
                severity_str = "info";
                break;
            }
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            {
                severity_str = "warning";
                break;
            }
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            {
                severity_str = "error";
                break;
            }
            default:
            {
                severity_str = "unknown";
                break;
            }
        }
        std::string type_str{};
        switch (static_cast<vk::DebugUtilsMessageTypeFlagsEXT::MaskType>(type))
        {
            case std::to_underlying(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral):
            {
                type_str = "general";
                break;
            }
            case std::to_underlying(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation):
            {
                type_str = "validation";
                break;
            }
            case std::to_underlying(vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance):
            {
                type_str = "performance";
                break;
            }
            default:
            {
                type_str = "unknown";
                break;
            }
        }
        log_file
        (
            "debug_message:\n"
            "    severity: " + severity_str + " (code: " + std::to_string(std::to_underlying(severity)) + ")\n"
            "    type: " + type_str +
                " (code: " + std::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT::MaskType>(type)) + ")\n"
            "    message_id: " + data->pMessageIdName + " (code: " + std::to_string(data->messageIdNumber) + ")\n"
            "    message: " + data->pMessage + "\n"
        );
        for (int i{ 0 }; i < static_cast<int>(data->objectCount); ++i)
        {
            std::string object_name{};
            const char* object_name_ptr{ data->pObjects[i].pObjectName };
            if (object_name_ptr)
            {
                object_name = object_name_ptr;
            }
            else
            {
                object_name = "[no name]";
            }
            log_file
            (
                "    object:\n"
                "        type: " "(code: " + std::to_string(std::to_underlying(data->pObjects[i].objectType)) + ")\n"
                "        handle: " + std::to_string(data->pObjects[i].objectHandle) + "\n"
                "        name: " + object_name + "\n"
            );
        }
        if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        {
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
    
    auto engine_t::find_eligible_queue_family
    (
        std::vector<vk::QueueFamilyProperties2>& queue_family_arr,
        const vk::raii::PhysicalDevice& device_physical
    )
    -> std::ranges::iterator_t<std::vector<vk::QueueFamilyProperties2>>
    {
        for (std::size_t i{ 0 }; i < queue_family_arr.size(); ++i)
        {
            //This variable tracks whether "device_physical" supports "queue_family_arr[i]"
            //and whether it has graphics capability or not.
            bool queue_family_support{ true };
            queue_family_support =
                queue_family_support &&
                (queue_family_arr[i].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics);
            queue_family_support =
                queue_family_support &&
                device_physical.getSurfaceSupportKHR(static_cast<std::uint32_t>(i), *m_surface);
            if (queue_family_support)
            {
                return queue_family_arr.begin() + i;
            }
        }
        return queue_family_arr.end();
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
        // GLFW windows are resizable by default, including maximize and restore.
        m_window = glfwCreateWindow
        (
            static_cast<int>(m_window_size.x),
            static_cast<int>(m_window_size.y),
            name.c_str(),
            nullptr,
            nullptr
        );

        int window_width{}, window_height{};
        glfwGetWindowSize(m_window, &window_width, &window_height);
        m_window_size = size_2D{ window_width, window_height };

        //Create an instance.
        vk::ApplicationInfo app_info
        {
            .pApplicationName{ name.c_str() },
            .applicationVersion{ Vulkan_version_encode(0, 0, 5, 0) },
            .pEngineName{ "lgo" },
            .engineVersion{ Vulkan_version_encode(0, 0, 5, 0) },
            .apiVersion{ Vulkan_version_encode(0, 1, 4, 0) }
        };
        std::vector<const char*> layer
        {
            #ifdef _DEBUG
            "VK_LAYER_KHRONOS_validation"
            #endif
        };
        std::uint32_t glfw_extension_count{};
        const char** glfw_extension_cstr{ glfwGetRequiredInstanceExtensions(&glfw_extension_count) };
        std::vector<const char*> extension_instance( glfw_extension_cstr, glfw_extension_cstr + glfw_extension_count );
        #ifdef _DEBUG
        extension_instance.push_back("VK_EXT_debug_utils");
        #endif
        extension_instance.push_back("VK_KHR_get_surface_capabilities2");
        vk::InstanceCreateInfo instance_info
        {
            .pApplicationInfo{ &app_info },
            .enabledLayerCount{ static_cast<std::uint32_t>(layer.size()) },
            .ppEnabledLayerNames{ layer.data() },
            .enabledExtensionCount{ static_cast<std::uint32_t>(extension_instance.size()) },
            .ppEnabledExtensionNames{ extension_instance.data() }
        };
        m_instance = vk::raii::Instance{ m_context, instance_info };

        //Create a debug messenger.
        #ifdef _DEBUG
        vk::DebugUtilsMessengerCreateInfoEXT messenger_info
        {
            .messageSeverity
            {
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
            },
            .messageType
            {
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
            },
            .pfnUserCallback{ &debug_callback },
            .pUserData{ this }
        };
        m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(messenger_info);
        #endif

        //Create a surface.
        //The window surface needs to be created right after the instance creation,
        //because it can actually influence the physical device selection.
        VkSurfaceKHR surface_c{};
        glfwCreateWindowSurface(*m_instance, m_window, nullptr, &surface_c);
        m_surface = vk::raii::SurfaceKHR{ m_instance, surface_c };

        //Create a physical device.
        std::vector<vk::raii::PhysicalDevice> device_available{ m_instance.enumeratePhysicalDevices() };
        std::multimap<int, vk::raii::PhysicalDevice> device_score{};
        std::vector<std::string> extension_device{ "VK_KHR_swapchain" };
        for (vk::raii::PhysicalDevice device : device_available)
        {
            vk::PhysicalDeviceProperties device_property{ device.getProperties() };
            std::vector<vk::QueueFamilyProperties2> device_queue_family{ device.getQueueFamilyProperties2() };
            std::vector<vk::ExtensionProperties> device_extension{ device.enumerateDeviceExtensionProperties() };
            auto device_feature
            {
                device.getFeatures2
                <
                    vk::PhysicalDeviceFeatures2,
                    vk::PhysicalDeviceVulkan11Features,
                    vk::PhysicalDeviceVulkan12Features,
                    vk::PhysicalDeviceVulkan13Features,
                    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
                >()
            };
            //This variable tracks whether "device" supports a queue family with graphics capability.
            if
            (
                device_property.apiVersion < Vulkan_version_encode(0, 1, 3, 0) ||
                find_eligible_queue_family
                (
                    device_queue_family,
                    device
                ) == device_queue_family.end() ||
                std::ranges::any_of
                (
                    extension_device,
                    [device_extension] (const std::string& extension) -> bool
                    {
                        return std::ranges::all_of
                        (
                            device_extension,
                            [extension] (const vk::ExtensionProperties& device_extension) -> bool
                            {
                                return device_extension.extensionName != extension;
                            }
                        );
                    }
                ) ||
                !device_feature.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters ||
                !device_feature.get<vk::PhysicalDeviceVulkan12Features>().runtimeDescriptorArray ||
                !device_feature.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering ||
                !device_feature.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 ||
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
            m_device_physical = device_score.rbegin()->second;
        }

        //Create a logical device.
        std::vector<vk::QueueFamilyProperties2> queue_family_arr{ m_device_physical.getQueueFamilyProperties2() };
        auto queue_family_iter
        {
            find_eligible_queue_family
            (
                queue_family_arr,
                m_device_physical
            )
        };
        float queue_priority{ 1.0 };
        std::uint32_t queue_family_index
        {
            static_cast<std::uint32_t>(
                std::ranges::distance
                (
                    queue_family_arr.begin(),
                    queue_family_iter
                )
            )
        };
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
            vk::PhysicalDeviceVulkan12Features
            {
                .runtimeDescriptorArray{ true }
            },
            vk::PhysicalDeviceVulkan13Features
            {
                .synchronization2{ true },
                .dynamicRendering{ true }
            },
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
            {
                .extendedDynamicState{ true }
            }
        };
        std::vector<const char*> extension_device_cstr_arr
        {
            std::from_range_t{},
            extension_device |
            std::ranges::views::transform
            (
                [] (const std::string& str) -> const char*
                {
                    return str.c_str();
                }
            )
        };
        vk::DeviceCreateInfo device_info
        {
            .pNext{ &feature.get<vk::PhysicalDeviceFeatures2>() },
            .queueCreateInfoCount{ 1 },
            .pQueueCreateInfos{ &device_queue },
            .enabledExtensionCount{ static_cast<std::uint32_t>(extension_device_cstr_arr.size()) },
            .ppEnabledExtensionNames{ extension_device_cstr_arr.data() }
        };
        m_device = vk::raii::Device{ m_device_physical, device_info };

        //Create a queue.
        m_command_queue = vk::raii::Queue{ m_device, queue_family_index, 0 };

        //Create a swapchain.
        vk::PhysicalDeviceSurfaceInfo2KHR surface_info
        {
            .surface{ *m_surface }
        };
        std::vector<vk::SurfaceFormat2KHR> surface_format_arr
        {
            m_device_physical.getSurfaceFormats2KHR(surface_info)
        };
        auto surface_format_it
        {
            std::ranges::find_if
            (
                surface_format_arr,
                [] (vk::SurfaceFormat2KHR format) -> bool
                {
                    return format.surfaceFormat.format == vk::Format::eB8G8R8A8Srgb &&
                        format.surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
                }
            )
        };
        m_surface_format = surface_format_it != surface_format_arr.end() ?
            *surface_format_it : surface_format_arr.front();
        //Create a command pool.
        vk::CommandPoolCreateInfo command_pool_info
        {
            .flags{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer },
            .queueFamilyIndex = queue_family_index
        };
        m_command_pool = vk::raii::CommandPool{ m_device, command_pool_info };

        //Create a command buffer.
        m_command_buffer = command_buffer_create();
        m_command_buffer.end();

        //Create semaphores and a fence.
        m_fence =
            vk::raii::Fence
            {
                m_device,
                {
                    .flags{ vk::FenceCreateFlagBits::eSignaled }
                }
            };

        recreate_swap_chain();

        glfwSetWindowUserPointer(m_window, reinterpret_cast<void*>(this));
        glfwSetCursorEnterCallback(m_window, GLFW_callback_cursor_enter);
        glfwSetCursorPosCallback(m_window, GLFW_callback_cursor_move);
        glfwSetMouseButtonCallback(m_window, GLFW_callback_mouse_button);
        glfwSetScrollCallback(m_window, GLFW_callback_wheel);
        glfwSetKeyCallback(m_window, GLFW_callback_key);
        glfwSetCharCallback(m_window, GLFW_callback_char);

        rect_primitive_t::init(this);
        m_device.waitIdle();
        // Register only after the swapchain and rendering resources are ready.
        glfwSetWindowRefreshCallback(m_window, GLFW_callback_window_refresh);
        return;
    }

    auto engine_t::recreate_swap_chain() -> bool
    {
        int width{}, height{};
        glfwGetFramebufferSize(m_window, &width, &height);
        if (width <= 0 || height <= 0)
        {
            return false;
        }
        // Retire GPU work before replacing attachments, command-buffer references, or semaphores.
        m_device.waitIdle();
        m_command_buffer.reset();
        vk::PhysicalDeviceSurfaceInfo2KHR surface_info{ .surface{ *m_surface } };
        vk::SurfaceCapabilities2KHR surface_capability
        {
            m_device_physical.getSurfaceCapabilities2KHR(surface_info)
        };
        if (surface_capability.surfaceCapabilities.currentExtent.width == std::numeric_limits<std::uint32_t>::max())
        {
            m_swap_chain_extent.width = std::clamp
            (
                static_cast<std::uint32_t>(width),
                surface_capability.surfaceCapabilities.minImageExtent.width,
                surface_capability.surfaceCapabilities.maxImageExtent.width
            );
            m_swap_chain_extent.height = std::clamp
            (
                static_cast<std::uint32_t>(height),
                surface_capability.surfaceCapabilities.minImageExtent.height,
                surface_capability.surfaceCapabilities.maxImageExtent.height
            );
        }
        else
        {
            m_swap_chain_extent = surface_capability.surfaceCapabilities.currentExtent;
        }
        std::uint32_t image_count_min{ surface_capability.surfaceCapabilities.minImageCount + 1 };
        if
        (
            surface_capability.surfaceCapabilities.maxImageCount &&
            (image_count_min > surface_capability.surfaceCapabilities.maxImageCount)
        )
        {
            image_count_min = surface_capability.surfaceCapabilities.maxImageCount;
        }
        vk::SwapchainCreateInfoKHR swapchain_info
        {
            .surface{ *m_surface },
            .minImageCount{ image_count_min },
            .imageFormat{ m_surface_format.surfaceFormat.format },
            .imageColorSpace{ m_surface_format.surfaceFormat.colorSpace },
            .imageExtent{ m_swap_chain_extent },
            .imageArrayLayers{ 1 },
            .imageUsage{ vk::ImageUsageFlagBits::eColorAttachment },
            .imageSharingMode{ vk::SharingMode::eExclusive },
            .preTransform{ surface_capability.surfaceCapabilities.currentTransform },
            .compositeAlpha{ vk::CompositeAlphaFlagBitsKHR::eOpaque },
            .presentMode{ vk::PresentModeKHR::eFifo },
            .clipped{ true },
            .oldSwapchain{ *m_swap_chain }
        };
        vk::raii::SwapchainKHR swap_chain{ m_device, swapchain_info };
        // Views must be released before their images and the retired swapchain.
        m_swap_chain_image_view.clear();
        m_depth_image_view = image_view_t{};
        m_depth_image = image_t{};
        m_semaphore_image.clear();
        m_semaphore_draw.clear();
        m_swap_chain = std::move(swap_chain);
        m_swap_chain_image = m_swap_chain.getImages();

        //Create image views.
        vk::ImageViewCreateInfo image_view_info
        {
            .viewType{ vk::ImageViewType::e2D },
            .format{ m_surface_format.surfaceFormat.format },
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

        //Create a depth buffer.
        m_depth_image.init
        ({
            .device_physical{ &m_device_physical },
            .device{ &m_device },
            .width{ m_swap_chain_extent.width },
            .height{ m_swap_chain_extent.height },
            .format{ vk::Format::eD32Sfloat },
            .usage{ vk::ImageUsageFlagBits::eDepthStencilAttachment },
            .property{ vk::MemoryPropertyFlagBits::eDeviceLocal }
        });
        m_depth_image_view.init
        ({
            .device{ &m_device },
            .image{ &m_depth_image.interface_get() },
            .format{ vk::Format::eD32Sfloat },
            .aspect{ vk::ImageAspectFlagBits::eDepth }
        });

        for (std::size_t i{ 0 }; i < m_swap_chain_image.size(); ++i)
        {
            m_semaphore_image.emplace_back(m_device, vk::SemaphoreCreateInfo{});
            m_semaphore_draw.emplace_back(m_device, vk::SemaphoreCreateInfo{});
        }
        m_framebuffer_size = size_2D{ width, height };
        m_swap_chain_dirty = false;
        return true;
    }

    auto engine_t::update_window_size() -> void
    {
        int width{}, height{};
        glfwGetWindowSize(m_window, &width, &height);
        if (width <= 0 || height <= 0 || (width == m_window_size.x && height == m_window_size.y))
        {
            return;
        }
        flush();
        m_window_size = size_2D{ width, height };
        if (m_callback_window_resize)
        {
            m_callback_window_resize(m_window_size);
        }
        // Even unchanged rectangles need new normalized coordinates; keep text in logical pixels.
        std::unique_lock lock{ m_rect_mutex };
        for (auto* rect : m_rect_primitive)
        {
            rect->upload_vertex_data();
        }
    }

    engine_t::~engine_t() noexcept
    {
        glfwSetWindowRefreshCallback(m_window, nullptr);
        const char* cleanup_error{};
        try
        {
            if (*m_device) m_device.waitIdle();
        }
        catch (const vk::DeviceLostError&)
        {
            //A lost device cannot finish GPU work. Continue destroying its resources;
            //letting Vulkan-Hpp's exception escape a destructor would call terminate().
            cleanup_error = "VK_ERROR_DEVICE_LOST while waiting for GPU shutdown.\n";
        }
        catch (...)
        {
            cleanup_error = "The GPU idle wait failed during engine cleanup.\n";
        }
        if (cleanup_error)
        {
            try
            {
                log_file(cleanup_error);
            }
            catch (...)
            {
                //Logging failures must not interrupt cleanup or replace an active exception.
            }
        }
        rect_primitive_t::uninit();
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return;
    }

    auto engine_t::message_loop() -> bool
    {
        glfwPollEvents();
        if (m_refresh_error)
        {
            std::rethrow_exception(std::exchange(m_refresh_error, {}));
        }
        if (glfwWindowShouldClose(m_window))
        {
            return true;
        }
        int width{}, height{};
        glfwGetFramebufferSize(m_window, &width, &height);
        if (glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) || width <= 0 || height <= 0)
        {
            // Minimized surfaces cannot acquire drawable images. Keep pumping close/restore events.
            glfwWaitEventsTimeout(0.05);
            return glfwWindowShouldClose(m_window);
        }
        render_frame();
        return glfwWindowShouldClose(m_window);
    }

    auto engine_t::render_frame() -> void
    {
        // Native messages can re-enter during layout or swapchain operations; never nest GPU work.
        if (m_rendering || m_refresh_error || glfwGetWindowAttrib(m_window, GLFW_ICONIFIED))
        {
            return;
        }
        m_rendering = true;
        try
        {
            update_window_size();
            // Refresh the displayed rate on both regular frames and Win32 modal-loop repaints.
            if (m_callback_frame_rate)
            {
                m_callback_frame_rate(frame_rate());
            }
            draw();
        }
        catch (...)
        {
            m_rendering = false;
            throw;
        }
        m_rendering = false;
    }

    auto engine_t::draw() -> void
    {
        int width{}, height{};
        glfwGetFramebufferSize(m_window, &width, &height);
        if (width <= 0 || height <= 0)
        {
            return;
        }
        if (width != m_framebuffer_size.x || height != m_framebuffer_size.y)
        {
            m_swap_chain_dirty = true;
        }
        if (m_swap_chain_dirty && !recreate_swap_chain())
        {
            return;
        }
        //Acquire next image.
		if
        (
            m_device.waitForFences
            (
                *m_fence,
                vk::True,
                std::numeric_limits<std::uint64_t>::max()
            ) !=
            vk::Result::eSuccess
        )
		{
			throw;
		}
        std::size_t semaphore_index{ m_frame_number % m_swap_chain_image.size() };
        vk::Result result{};
        std::uint32_t image_index{};
        try
        {
            std::tie(result, image_index) = m_swap_chain.acquireNextImage
            (
                std::numeric_limits<std::uint64_t>::max(),
                *m_semaphore_image[semaphore_index],
                nullptr
            );
        }
        catch (const vk::OutOfDateKHRError&)
        {
            // Leave the fence signaled: no submission will signal it on this path.
            m_swap_chain_dirty = true;
            return;
        }
        m_swap_chain_dirty = result == vk::Result::eSuboptimalKHR;

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
        m_depth_image.barrier
        (
            m_command_buffer,
            vk::ImageAspectFlagBits::eDepth,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests
        );
        vk::RenderingAttachmentInfo color_attachment_info
        {
            .imageView{ m_swap_chain_image_view[image_index] },
            .imageLayout{ vk::ImageLayout::eColorAttachmentOptimal },
            .loadOp{ vk::AttachmentLoadOp::eClear },
            .storeOp{ vk::AttachmentStoreOp::eStore },
            .clearValue{ vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f) }
        };
        vk::RenderingAttachmentInfo depth_attachment_info
        {
            .imageView{ m_depth_image_view.interface_get() },
            .imageLayout{ vk::ImageLayout::eDepthAttachmentOptimal },
            .loadOp{ vk::AttachmentLoadOp::eClear },
            .storeOp{ vk::AttachmentStoreOp::eDontCare },
            .clearValue{ vk::ClearDepthStencilValue(1.0f, 0) }
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
            .pColorAttachments{ &color_attachment_info },
            .pDepthAttachment{ &depth_attachment_info }
        };
        m_command_buffer.beginRendering(render_info);
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
        std::unique_lock rect_lock{ m_rect_mutex };
        rect_primitive_t::render(m_rect_primitive, m_command_buffer);
        rect_lock.unlock();
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

        // Present waits are tied to acquired images, so reuse the draw semaphore by image index.
        //Submit the command buffer.
        vk::PipelineStageFlags stage_wait{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
        const vk::SubmitInfo submit_info
        {
            .waitSemaphoreCount{ 1 },
            .pWaitSemaphores{ &*m_semaphore_image[semaphore_index] },
            .pWaitDstStageMask{ &stage_wait },
            .commandBufferCount{ 1 },
            .pCommandBuffers{ &*m_command_buffer },
            .signalSemaphoreCount{ 1 },
            .pSignalSemaphores{ &*m_semaphore_draw[image_index] }
        };
        // Reset only when a successful acquisition is followed by an actual submission.
        m_device.resetFences(*m_fence);
        m_command_queue.submit(submit_info, *m_fence);

        //Present the image.
        const vk::PresentInfoKHR present_info
        {
            .waitSemaphoreCount{ 1 },
            .pWaitSemaphores{ &*m_semaphore_draw[image_index] },
            .swapchainCount{ 1 },
            .pSwapchains{ &*m_swap_chain },
            .pImageIndices{ &image_index }
        };
        try
        {
            result = m_command_queue.presentKHR(present_info);
            m_swap_chain_dirty = m_swap_chain_dirty || result == vk::Result::eSuboptimalKHR;
            // Count presented frames, not event-loop iterations or failed/minimized draws.
            const auto now{ std::chrono::steady_clock::now() };
            while (!m_frame_time.empty() && m_frame_time.front() <= now - std::chrono::seconds{ 1 })
            {
                m_frame_time.pop_front();
            }
            m_frame_time.push_back(now);
        }
        catch (const vk::OutOfDateKHRError&)
        {
            m_swap_chain_dirty = true;
        }

        ++m_frame_number;
        return;
    }

    auto engine_t::flush() -> void
    {
        m_command_queue.waitIdle();
        return;
    }

    auto engine_t::log(std::uint32_t tab_count) const -> void
    {
        log_file(tab(tab_count) + "{\n");
        log_file(tab(tab_count + 1) + "type: engine_t\n");
        log_file(tab(tab_count + 1) + "name: " + m_name + "\n");
        log_file(tab(tab_count + 1) + "rect_primitive:\n");
        log_file(tab(tab_count + 1) + "[\n");
        for (auto i{ m_rect_primitive.begin() }; i != m_rect_primitive.end(); ++i)
        {
            (*i)->log(tab_count + 2);
        }
        log_file(tab(tab_count + 1) + "]\n");
        log_file(tab(tab_count) + "}\n");
        return;
    }

    auto engine_t::format_get() const -> vk::Format
    {
        return m_surface_format.surfaceFormat.format;
    }

    auto engine_t::set_exit() -> void
    {
        m_exit = true;
        return;
    }

    auto engine_t::get_exit() -> bool
    {
        return m_exit;
    }

    auto engine_t::get_window_pos() -> pos_2D
    {
        return m_window_pos;
    }

    auto engine_t::get_window_size() -> const size_2D&
    {
        return m_window_size;
    }

    auto engine_t::get_framebuffer_size() const -> size_2D
    {
        return size_2D{ m_swap_chain_extent.width, m_swap_chain_extent.height };
    }

    auto engine_t::callback_window_resize_set(std::function<void(size_2D)> callback) -> void
    {
        m_callback_window_resize = std::move(callback);
    }

    auto engine_t::callback_frame_rate_set(std::function<void(std::uint64_t)> callback) -> void
    {
        m_callback_frame_rate = std::move(callback);
    }

    auto engine_t::get_cursor_pos() -> pos_2D
    {
        double cursor_pos_x_double{}, cursor_pos_y_double{};
        glfwGetCursorPos(m_window, &cursor_pos_x_double, &cursor_pos_y_double);
        return pos_2D{ static_cast<int64_t>(cursor_pos_x_double), static_cast<int64_t>(cursor_pos_y_double) };
    }

    auto engine_t::frame_rate() const -> std::uint64_t
    {
        const auto cutoff{ std::chrono::steady_clock::now() - std::chrono::seconds{ 1 } };
        return static_cast<std::uint64_t>(std::ranges::count_if(m_frame_time,
            [cutoff](auto time) { return time > cutoff; }));
    }

    auto engine_t::device_get() -> vk::raii::Device&
    {
        return m_device;
    }
    
    auto engine_t::window_get() -> GLFWwindow*
    {
        return m_window;
    }

    auto engine_t::device_physical_get() -> vk::raii::PhysicalDevice&
    {
        return m_device_physical;
    }

    auto engine_t::command_queue_get() -> vk::raii::Queue&
    {
        return m_command_queue;
    }

    auto engine_t::command_buffer_create() -> vk::raii::CommandBuffer
    {
        vk::CommandBufferAllocateInfo command_buffer_info
        {
            .commandPool{ m_command_pool },
            .level{ vk::CommandBufferLevel::ePrimary },
            .commandBufferCount{ 1 }
        };
        vk::raii::CommandBuffer buffer
        {
            std::move(vk::raii::CommandBuffers{ m_device, command_buffer_info }.front())
        };
        vk::CommandBufferBeginInfo begin_info
        {
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        };
		buffer.begin(begin_info);
        return std::move(buffer);
    }

    auto engine_t::add_rect(const rect_t& rect) -> rect_primitive_t*
    {
        rect_primitive_t* out{ new rect_primitive_t{ rect } };
        std::unique_lock lock{ m_rect_mutex };
        m_rect_primitive.emplace(out);
        return out;
    }

    auto engine_t::remove_rect(rect_primitive_t* in) -> void
    {
        std::unique_lock lock{ m_rect_mutex };
        m_rect_primitive.erase(in);
        flush();
        delete in;
        return;
    }
    
    auto engine_t::cursor_pos_get() const -> pos_2D
    {
        double x{}, y{};
        glfwGetCursorPos(m_window, &x, &y);
        return pos_2D{ static_cast<std::int64_t>(x), static_cast<std::int64_t>(y) };
    }

    auto engine_t::callback_mouse_enter_add(callback_mouse_enter_t callback) -> callback_mouse_enter_t*
    {
        auto out{ new callback_mouse_enter_t{ callback } };
        if (m_callback_mouse_enter.m_calling_callback)
        {
            m_callback_mouse_enter.m_pending.emplace(out);
        }
        else
        {
            m_callback_mouse_enter.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::callback_mouse_enter_remove(callback_mouse_enter_t* in) -> void
    {
        if (!m_callback_mouse_enter.m_effective.erase(in))
        {
            m_callback_mouse_enter.m_pending.erase(in);
        }
        delete in;
        return;
    }

    auto engine_t::callback_mouse_move_add(callback_mouse_move_t callback) -> callback_mouse_move_t*
    {
        auto out{ new callback_mouse_move_t{ callback } };
        if (m_callback_mouse_move.m_calling_callback)
        {
            m_callback_mouse_move.m_pending.emplace(out);
        }
        else
        {
            m_callback_mouse_move.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::callback_mouse_move_remove(callback_mouse_move_t* in) -> void
    {
        if (!m_callback_mouse_move.m_effective.erase(in))
        {
            m_callback_mouse_move.m_pending.erase(in);
        }
        delete in;
        return;
    }

    auto engine_t::callback_mouse_button_add(callback_mouse_button_t callback) -> callback_mouse_button_t*
    {
        auto out{ new callback_mouse_button_t{ callback } };
        if (m_callback_mouse_button.m_calling_callback)
        {
            m_callback_mouse_button.m_pending.emplace(out);
        }
        else
        {
            m_callback_mouse_button.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::callback_mouse_button_remove(callback_mouse_button_t* in) -> void
    {
        if (!m_callback_mouse_button.m_effective.erase(in))
        {
            m_callback_mouse_button.m_pending.erase(in);
        }
        delete in;
        return;
    }

    auto engine_t::callback_mouse_scroll_add(callback_mouse_scroll_t callback) -> callback_mouse_scroll_t*
    {
        auto out{ new callback_mouse_scroll_t{ callback } };
        if (m_callback_mouse_scroll.m_calling_callback)
        {
            m_callback_mouse_scroll.m_pending.emplace(out);
        }
        else
        {
            m_callback_mouse_scroll.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::callback_mouse_scroll_remove(callback_mouse_scroll_t* in) -> void
    {
        if (!m_callback_mouse_scroll.m_effective.erase(in))
        {
            m_callback_mouse_scroll.m_pending.erase(in);
        }
        delete in;
        return;
    }

    auto engine_t::callback_key_add(callback_key_t callback) -> callback_key_t*
    {
        auto out{ new callback_key_t{ callback } };
        if (m_callback_key.m_calling_callback)
        {
            m_callback_key.m_pending.emplace(out);
        }
        else
        {
            m_callback_key.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::callback_key_remove(callback_key_t* in) -> void
    {
        if (!m_callback_key.m_effective.erase(in))
        {
            m_callback_key.m_pending.erase(in);
        }
        delete in;
        return;
    }

    auto engine_t::callback_cha_add(callback_cha_t callback) -> callback_cha_t*
    {
        auto out{ new callback_cha_t{ callback } };
        if (m_callback_cha.m_calling_callback)
        {
            m_callback_cha.m_pending.emplace(out);
        }
        else
        {
            m_callback_cha.m_effective.emplace(out);
        }
        return out;
    }

    auto engine_t::callback_cha_remove(callback_cha_t* in) -> void
    {
        if (!m_callback_cha.m_effective.erase(in))
        {
            m_callback_cha.m_pending.erase(in);
        }
        delete in;
        return;
    }
}
