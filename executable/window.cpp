#include "window.h"
#include "value.h"
#include "debug_f.h"

void Message_window_pv::destruct(Data_pv* data) {
	destruct1(data);
	return;
}

GLFWwindow* create_window(std::string title, unsigned int width, unsigned int height,
	bool resizeable, Data_pv* data) {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, constant::OpenGL_version_major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, constant::OpenGL_version_minor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, resizeable);
	GLFWwindow* window{ glfwCreateWindow(width, height, title.data(), nullptr, nullptr) };
	if (!window) {
		handle_error(U"error: Failed to open window.\n", data);
		std::abort();
	}
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		handle_error(U"error: Failed to initialize GLAD.\n", data);
		std::abort();
	}
	glViewport(0, 0, width, height);
	return window;
}