module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

export module external.GLFW;

import external.Vulkan;

export {
    using ::GLFWwindow;
    
    using ::glfwInit;
    using ::glfwTerminate;

    using ::glfwGetRequiredInstanceExtensions;
    using ::glfwCreateWindowSurface;

    using ::glfwCreateWindow;
    using ::glfwDestroyWindow;
    using ::glfwWindowHint;
    using ::glfwGetFramebufferSize;
    using ::glfwGetCursorPos;
    using ::glfwGetWindowPos;
    using ::glfwPostEmptyEvent;
    using ::glfwSetWindowShouldClose;
    using ::glfwSetWindowUserPointer;
    using ::glfwGetWindowUserPointer;
    using ::glfwSetCursorPosCallback;
    using ::glfwSetCursorEnterCallback;
    using ::glfwSetMouseButtonCallback;
    using ::glfwSetScrollCallback;
    using ::glfwSetKeyCallback;
    using ::glfwSetCharCallback;

    using ::glfwPollEvents;
    using ::glfwWindowShouldClose;

    inline auto glfwCreateVulkanWindowSurface(vk::Instance instance, GLFWwindow* window) -> vk::SurfaceKHR {
        VkSurfaceKHR surface{};
        if (::glfwCreateWindowSurface(static_cast<VkInstance>(instance), window, nullptr, &surface) != VK_SUCCESS) {
            return nullptr;
        }
        return vk::SurfaceKHR{ surface };
    }

    inline constexpr int glfw_client_api{ GLFW_CLIENT_API };
    inline constexpr int glfw_no_api{ GLFW_NO_API };
    inline constexpr int glfw_resizable{ GLFW_RESIZABLE };
    inline constexpr int glfw_false{ GLFW_FALSE };
    inline constexpr int glfw_mouse_button_left{ GLFW_MOUSE_BUTTON_LEFT };
    inline constexpr int glfw_press{ GLFW_PRESS };
    inline constexpr int glfw_release{ GLFW_RELEASE };
    inline constexpr int glfw_repeat{ GLFW_REPEAT };
}
