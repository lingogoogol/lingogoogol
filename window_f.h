#ifndef WINDOW_F_H
#define WINDOW_F_H

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

enum class Message_window_type;

class Message_window_pv;
template<Message_window_type T1>
class Message_window;

GLFWwindow* create_window(std::string title, unsigned int width, unsigned int height, bool resizeable);

#endif