module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

export module external.GLFW;

export {
    using ::GLFWwindow;
    
    using ::glfwInit;
    using ::glfwTerminate;

    using ::glfwGetRequiredInstanceExtensions;

    using ::glfwCreateWindow;
    using ::glfwDestroyWindow;
    using ::glfwWindowHint;

    using ::glfwPollEvents;
    using ::glfwWindowShouldClose;
}
