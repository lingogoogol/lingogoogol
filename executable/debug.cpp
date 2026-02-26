#include <glad/glad.h>

#include "debug.h"
#include "value.h"
#include "window.h"

void handle_error1(const char* file, int line, std::u32string message) {
    object::logfile << "Error:\n" << "    File: " << file << "\n    Line: " << line << "\n    Message: " << message.data() << "\n";
    Message_window<Message_window_type::Error> window{ "Error",message };
}

void check_GL_error1(const char* file, int line) {
    GLenum error{};
    while ((error = glGetError()) != GL_NO_ERROR)
        object::logfile << "Error:\n" << "    File: " << file << "\n    Line: " << line << "\n    Code: " << error << "\n";
}