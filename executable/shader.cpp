#include <glad/glad.h>

#include "value_f.h"
#include "debug_f.h"
#include "shader.h"
#include "string_f.h"

unsigned int create_shader(std::string vertex, std::string fragment, Data_pv* data) {
    unsigned int vertex_shader{ glCreateShader(GL_VERTEX_SHADER) };
    const char* shader_code{ &(vertex[0]) };
    glShaderSource(vertex_shader, 1, &shader_code, NULL);
    glCompileShader(vertex_shader);
    int success{};
    char error_message[constant::error_message_max_size]{};
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, constant::error_message_max_size, NULL, error_message);
        handle_error(to_u32string(error_message), data);
    }
    unsigned int fragment_shader{ glCreateShader(GL_FRAGMENT_SHADER) };
    shader_code = &(fragment[0]);
    glShaderSource(fragment_shader, 1, &shader_code, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, constant::error_message_max_size, NULL, error_message);
        handle_error(to_u32string(error_message), data);
    }
    unsigned int shader{ glCreateProgram() };
    glAttachShader(shader, vertex_shader);
    glAttachShader(shader, fragment_shader);
    glLinkProgram(shader);
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader, constant::error_message_max_size, NULL, error_message);
        handle_error(to_u32string(error_message), data);
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return shader;
}