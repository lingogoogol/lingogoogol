#ifndef LGO_GRAPHIC_F_H
#define LGO_GRAPHIC_F_H

#include <string>

#include "../lib/glad.h"
#include "../lib/glfw.h"
#include "../holder_f.h"

namespace lgo {
	enum class message_window_type;

	class message_window_pv;
	template<message_window_type T1>
	class message_window;

	GLFWwindow* create_window(std::string title, unsigned int width,
		unsigned int height, bool resizeable);
	template<typename... T1>
	unsigned int create_fbo(T1... attachments);
	unsigned int create_shader(std::string vertex, std::string fragment);
	template<typename T1>
	unsigned int create_tex(T1* tex_data, GLenum format,
		unsigned int width, unsigned int height);
	int get_window_width(GLFWwindow* window);
	int get_window_height(GLFWwindow* window);
	int get_framebuffer_width(GLFWwindow* window);
	int get_framebuffer_height(GLFWwindow* window);
	double get_cursor_x(GLFWwindow* window);
	double get_cursor_y(GLFWwindow* window);
	void set_window_width(GLFWwindow* window, int width);
	void set_window_height(GLFWwindow* window, int height);
	void set_cursor_x(GLFWwindow* window, double x);
	void set_cursor_y(GLFWwindow* window, double y);
	void set_callback(GLFWwindow* window, GLFWkeyfun key_callback,
		GLFWcharfun char_callback, GLFWcursorposfun cursor_pos_callback,
		GLFWmousebuttonfun mouse_button_callback, GLFWscrollfun scroll_callback);

	namespace impl {
		template<typename... T1>
		unsigned int create_fbo1(int color_attachment_num,
			unsigned int attachment, T1... attachments);
	}
}

#endif