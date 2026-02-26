#ifndef LIBRARY_GRAPHIC_F_H
#define LIBRARY_GRAPHIC_F_H

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "holder_f.h"

enum class Message_window_type;

class Message_window_pv;
template<Message_window_type T1>
class Message_window;

unsigned int create_shader(std::string vertex, std::string fragment);
GLFWwindow* create_window(std::string title, unsigned int width, unsigned int height, bool resizeable);
int get_window_width(GLFWwindow* window);
int get_window_height(GLFWwindow* window);
double get_cursor_x(GLFWwindow* window);
void set_cursor_x(GLFWwindow* window, double x);
double get_cursor_y(GLFWwindow* window);
void set_cursor_y(GLFWwindow* window, double y);
void set_callback(GLFWwindow* window, GLFWkeyfun key_callback, GLFWcharfun char_callback,
    GLFWcursorposfun cursor_pos_callback, GLFWmousebuttonfun mouse_button_callback, GLFWscrollfun scroll_callback);

#endif