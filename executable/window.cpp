#include "window.h"
#include "value.h"

void Message_window_pv::destruct() {
	destruct1();
	return;
}

GLFWwindow* create_window(std::string title, unsigned int width, unsigned int height, bool resizeable) {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, constant::OpenGL_version_major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, constant::OpenGL_version_minor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, resizeable);
	GLFWwindow* window{ glfwCreateWindow(width, height, title.data(), nullptr, nullptr) };
	if (!window) {
		object::logfile << "error: Failed to open window.\n";
		std::abort();
	}
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		object::logfile << "error: Failed to initialize GLAD.\n";
		std::abort();
	}
	glViewport(0, 0, width, height);
	return window;
}