#pragma once
// Android-only additions to the GLFW API implemented in glfw.cpp. The desktop
// GLFW model has no surface that disappears while the app keeps running, no
// system bars drawn over the window and no on-screen keyboard.
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

// Called from glfwPollEvents/glfwWaitEventsTimeout on the app thread.
// available == GLFW_FALSE: the activity's surface is being destroyed; release
// every Vulkan object that references it before returning (the swapchain and
// the VkSurfaceKHR). available == GLFW_TRUE: a new surface exists; create a new
// VkSurfaceKHR with glfwCreateWindowSurface.
typedef void (*GLFWsurfacefunANDROID)(GLFWwindow* window, int available);
GLFWAPI GLFWsurfacefunANDROID glfwSetWindowSurfaceCallbackANDROID(GLFWwindow* window,
                                                                  GLFWsurfacefunANDROID callback);

// The part of the framebuffer not covered by system bars or display cutouts,
// in framebuffer pixels. glfwGetWindowSize reports this area in logical pixels.
GLFWAPI void glfwGetWindowContentAreaANDROID(GLFWwindow* window, int* x, int* y, int* width, int* height);

// Shows or hides the soft keyboard while a text field has focus.
GLFWAPI void glfwSetTextInputActiveANDROID(GLFWwindow* window, int active);

// Asked when a finger touches down, with the cursor position in logical pixels.
// Nonzero: the touch acts as a mouse drag (button press, cursor moves, button
// release), e.g. to drag a scrollbar. Zero: a tap clicks and a drag scrolls the
// area under the finger through scroll (wheel) events.
typedef int (*GLFWtouchdragfunANDROID)(GLFWwindow* window, double x, double y);
GLFWAPI GLFWtouchdragfunANDROID glfwSetTouchDragCallbackANDROID(GLFWwindow* window,
                                                               GLFWtouchdragfunANDROID callback);

// Delivers a press and release of a GLFW key, e.g. the system Back gesture as
// GLFW_KEY_ESCAPE. Call on the app thread.
GLFWAPI void glfwPressKeyANDROID(int key);

#ifdef __cplusplus
}
#endif
