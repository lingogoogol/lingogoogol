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
constexpr auto GLFW_PRESS_export{ GLFW_PRESS };
#undef GLFW_PRESS
constexpr auto GLFW_RELEASE_export{ GLFW_RELEASE };
#undef GLFW_RELEASE

export
{
    using ::VkSurfaceKHR;
    
    using ::GLFWwindow;
    
    using ::glfwInit;
    using ::glfwTerminate;

    using ::glfwGetRequiredInstanceExtensions;

    using ::glfwCreateWindow;
    using ::glfwDestroyWindow;
    using ::glfwWindowHint;
    using ::glfwCreateWindowSurface;
    using ::glfwGetFramebufferSize;
    
    using ::glfwGetCursorPos;
    using ::glfwGetMouseButton;

    using ::glfwPollEvents;
    using ::glfwWindowShouldClose;

    constexpr auto GLFW_CLIENT_API{ GLFW_CLIENT_API_export };
    constexpr auto GLFW_NO_API{ GLFW_NO_API_export };
    constexpr auto GLFW_RESIZABLE{ GLFW_RESIZABLE_export };
    constexpr auto GLFW_FALSE{ GLFW_FALSE_export };
    constexpr auto GLFW_MOUSE_BUTTON_LEFT{ GLFW_MOUSE_BUTTON_LEFT_export };
    constexpr auto GLFW_PRESS{ GLFW_PRESS_export };
    constexpr auto GLFW_RELEASE{ GLFW_RELEASE_export };
}
