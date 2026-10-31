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
constexpr auto GLFW_KEY_ESCAPE_export{ GLFW_KEY_ESCAPE };
#undef GLFW_KEY_ESCAPE
constexpr auto GLFW_KEY_F3_export{ GLFW_KEY_F3 };
#undef GLFW_KEY_F3
constexpr auto GLFW_PRESS_export{ GLFW_PRESS };
#undef GLFW_PRESS
constexpr auto GLFW_RELEASE_export{ GLFW_RELEASE };
#undef GLFW_RELEASE

constexpr auto GLFW_HOVERED_export{ GLFW_HOVERED };
#undef GLFW_HOVERED

constexpr auto GLFW_ICONIFIED_export{ GLFW_ICONIFIED };
#undef GLFW_ICONIFIED

constexpr auto GLFW_DONT_CARE_export{ GLFW_DONT_CARE };
#undef GLFW_DONT_CARE

constexpr auto GLFW_REPEAT_export{ GLFW_REPEAT };
#undef GLFW_REPEAT
constexpr auto GLFW_KEY_BACKSPACE_export{ GLFW_KEY_BACKSPACE };
#undef GLFW_KEY_BACKSPACE
constexpr auto GLFW_KEY_DELETE_export{ GLFW_KEY_DELETE };
#undef GLFW_KEY_DELETE
constexpr auto GLFW_MOD_CONTROL_export{ GLFW_MOD_CONTROL };
#undef GLFW_MOD_CONTROL
constexpr auto GLFW_KEY_A_export{ GLFW_KEY_A };
#undef GLFW_KEY_A
constexpr auto GLFW_KEY_C_export{ GLFW_KEY_C };
#undef GLFW_KEY_C
constexpr auto GLFW_KEY_V_export{ GLFW_KEY_V };
#undef GLFW_KEY_V

constexpr auto GLFW_KEY_LEFT_SHIFT_export{ GLFW_KEY_LEFT_SHIFT };
#undef GLFW_KEY_LEFT_SHIFT
constexpr auto GLFW_KEY_RIGHT_SHIFT_export{ GLFW_KEY_RIGHT_SHIFT };
#undef GLFW_KEY_RIGHT_SHIFT

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
    using ::glfwFocusWindow;
    using ::glfwGetWindowAttrib;
    using ::glfwSetWindowSizeLimits;
    using ::glfwSetWindowRefreshCallback;
    using ::glfwSetWindowUserPointer;
    using ::glfwGetWindowUserPointer;
    
    using ::glfwGetCursorPos;
    using ::glfwGetMouseButton;
    using ::glfwGetKey;
    using ::glfwGetClipboardString;
    using ::glfwSetClipboardString;
    using ::glfwSetCursorPosCallback;
    using ::glfwSetCursorEnterCallback;
    using ::glfwSetMouseButtonCallback;
    using ::glfwSetScrollCallback;
    using ::glfwSetKeyCallback;
    using ::glfwSetCharCallback;

    using ::glfwPollEvents;
    using ::glfwWaitEventsTimeout;
    using ::glfwWindowShouldClose;

    constexpr auto GLFW_REPEAT{ GLFW_REPEAT_export };
    constexpr auto GLFW_KEY_BACKSPACE{ GLFW_KEY_BACKSPACE_export };
    constexpr auto GLFW_KEY_DELETE{ GLFW_KEY_DELETE_export };
    constexpr auto GLFW_MOD_CONTROL{ GLFW_MOD_CONTROL_export };
    constexpr auto GLFW_KEY_A{ GLFW_KEY_A_export };
    constexpr auto GLFW_KEY_C{ GLFW_KEY_C_export };
    constexpr auto GLFW_KEY_V{ GLFW_KEY_V_export };

    constexpr auto GLFW_HOVERED{ GLFW_HOVERED_export };
    constexpr auto GLFW_ICONIFIED{ GLFW_ICONIFIED_export };
    constexpr auto GLFW_DONT_CARE{ GLFW_DONT_CARE_export };
    constexpr auto GLFW_CLIENT_API{ GLFW_CLIENT_API_export };
    constexpr auto GLFW_NO_API{ GLFW_NO_API_export };
    constexpr auto GLFW_RESIZABLE{ GLFW_RESIZABLE_export };
    constexpr auto GLFW_FALSE{ GLFW_FALSE_export };
    constexpr auto GLFW_MOUSE_BUTTON_LEFT{ GLFW_MOUSE_BUTTON_LEFT_export };
    constexpr auto GLFW_KEY_LEFT_SHIFT{ GLFW_KEY_LEFT_SHIFT_export };
    constexpr auto GLFW_KEY_RIGHT_SHIFT{ GLFW_KEY_RIGHT_SHIFT_export };
    constexpr auto GLFW_KEY_ESCAPE{ GLFW_KEY_ESCAPE_export };
    constexpr auto GLFW_KEY_F3{ GLFW_KEY_F3_export };
    constexpr auto GLFW_PRESS{ GLFW_PRESS_export };
    constexpr auto GLFW_RELEASE{ GLFW_RELEASE_export };
}
