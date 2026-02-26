#ifndef LIBRARY_GRAPHIC_D_H
#define LIBRARY_GRAPHIC_D_H

#include <glad/glad.h>

#include "graphic.h"
#include "debug_f.h"
#include "math_f.h"
#include "const_f.h"
#include "data_f.h"

inline void Message_window_pv::destruct() {
    destruct1();
    return;
}

inline unsigned int create_shader(std::string vertex, std::string fragment) {
    unsigned int vertex_shader{ glCreateShader(GL_VERTEX_SHADER) };
    const char* shader_code{ &(vertex[0]) };
    glShaderSource(vertex_shader, 1, &shader_code, NULL);
    glCompileShader(vertex_shader);
    int success{};
    char error_message[implement::error_message_max_size]{};
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, implement::error_message_max_size, NULL, error_message);
        handle_error(to_string32(error_message));
    }
    unsigned int fragment_shader{ glCreateShader(GL_FRAGMENT_SHADER) };
    shader_code = &(fragment[0]);
    glShaderSource(fragment_shader, 1, &shader_code, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, implement::error_message_max_size, NULL, error_message);
        handle_error(to_string32(error_message));
    }
    unsigned int shader{ glCreateProgram() };
    glAttachShader(shader, vertex_shader);
    glAttachShader(shader, fragment_shader);
    glLinkProgram(shader);
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader, implement::error_message_max_size, NULL, error_message);
        handle_error(to_string32(error_message));
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return shader;
}

inline GLFWwindow* create_window(std::string title, unsigned int width, unsigned int height, bool resizeable) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, resizeable);
    GLFWwindow* window{ glfwCreateWindow(width, height, title.data(), nullptr, nullptr) };
    if (!window) {
        handle_error(U"error: Failed to open window.\n");
        std::abort();
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        handle_error(U"error: Failed to initialize GLAD.\n");
        std::abort();
    }
    glViewport(0, 0, width, height);
    return window;
}

#endif