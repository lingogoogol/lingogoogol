#ifndef GRAPHIC_F_H
#define GRAPHIC_F_H

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "data_f.h"

enum class Message_window_type;

class Message_window_pv;

GLFWwindow* create_window(std::string title, unsigned int width, unsigned int height,
	bool resizeable, Data_pv* data);

unsigned int create_shader(std::string vertex, std::string fragment, Data_pv* data);

#endif