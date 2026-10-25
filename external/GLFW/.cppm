module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

export module external.GLFW;

constexpr auto GLFW_CLIENT_API_export{ GLFW_CLIENT_API };
#undef GLFW_CLIENT_API
constexpr auto GLFW_NO_API_export{ GLFW_NO_API };
#undef GLFW_NO_API
constexpr auto GLFW_RESIZABLE_export{ GLFW_RESIZABLE };
#undef GLFW_RESIZABLE
constexpr auto GLFW_FALSE_export{ GLFW_FALSE };
#undef GLFW_FALSE
constexpr auto GLFW_MOUSE_BUTTON_LEFT_export{ GLFW_MOUSE_BUTTON_LEFT };
#undef GLFW_MOUSE_BUTTON_LEFT
constexpr auto GLFW_KEY_F3_export{ GLFW_KEY_F3 };
#undef GLFW_KEY_F3
constexpr auto GLFW_PRESS_export{ GLFW_PRESS };
#undef GLFW_PRESS
constexpr auto GLFW_RELEASE_export{ GLFW_RELEASE };
#undef GLFW_RELEASE

constexpr auto GLFW_ICONIFIED_export{ GLFW_ICONIFIED };
#undef GLFW_ICONIFIED

constexpr auto GLFW_DONT_CARE_export{ GLFW_DONT_CARE };
#undef GLFW_DONT_CARE

export
{
    using ::VkSurfaceKHR;
    
    using ::GLFWwindow;
    
    using ::glfwInit;
    using ::glfwVulkanSupported;
    using ::glfwGetError;
    using ::glfwTerminate;

    using ::glfwGetRequiredInstanceExtensions;

    using ::glfwCreateWindow;
    using ::glfwDestroyWindow;
    using ::glfwWindowHint;
    using ::glfwCreateWindowSurface;
    using ::glfwGetFramebufferSize;
    using ::glfwGetWindowSize;
    using ::glfwSetWindowSize;
    using ::glfwGetWindowAttrib;
    using ::glfwSetWindowSizeLimits;
    using ::glfwSetWindowRefreshCallback;
    using ::glfwSetWindowUserPointer;
    using ::glfwGetWindowUserPointer;
    
    using ::glfwGetCursorPos;
    using ::glfwGetMouseButton;
    using ::glfwGetKey;
    using ::glfwSetCursorPosCallback;
    using ::glfwSetCursorEnterCallback;
    using ::glfwSetMouseButtonCallback;
    using ::glfwSetScrollCallback;
    using ::glfwSetKeyCallback;
    using ::glfwSetCharCallback;

    using ::glfwPollEvents;
    using ::glfwWaitEventsTimeout;
    using ::glfwWindowShouldClose;

    constexpr auto GLFW_ICONIFIED{ GLFW_ICONIFIED_export };
    constexpr auto GLFW_DONT_CARE{ GLFW_DONT_CARE_export };
    constexpr auto GLFW_CLIENT_API{ GLFW_CLIENT_API_export };
    constexpr auto GLFW_NO_API{ GLFW_NO_API_export };
    constexpr auto GLFW_RESIZABLE{ GLFW_RESIZABLE_export };
    constexpr auto GLFW_FALSE{ GLFW_FALSE_export };
    constexpr auto GLFW_MOUSE_BUTTON_LEFT{ GLFW_MOUSE_BUTTON_LEFT_export };
    constexpr auto GLFW_KEY_F3{ GLFW_KEY_F3_export };
    constexpr auto GLFW_PRESS{ GLFW_PRESS_export };
    constexpr auto GLFW_RELEASE{ GLFW_RELEASE_export };
}
