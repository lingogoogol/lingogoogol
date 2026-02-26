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

#endif