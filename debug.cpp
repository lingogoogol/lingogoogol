#include <functional>
#include <glad/glad.h>

#include "debug.h"
#include "data.h"
#include "graphic.h"
#include "math_f.h"
#include "holder.h"

void handle_error1(const char* file, int line, std::u32string message, Data_pv* data) {
    *data->logfile << "Error:\n" << "    File: " << file << "\n    Line: " << line << "\n    Message: " << to_string8(message.data()) << "\n";
    data->message_window.push_back(Copy_holder{ static_cast<Message_window_pv*>(
        new Message_window<Message_window_type::Error>{ "Error", message, data }),
        std::function<void(Message_window_pv*)>{std::bind(std::mem_fn(&Message_window_pv::destruct),
        std::placeholders::_1, data)},data });
}

void check_GL_error1(const char* file, int line, Data_pv* data) {
    GLenum error{};
    while ((error = glGetError()) != GL_NO_ERROR)
        *data->logfile << "Error:\n" << "    File: " << file << "\n    Line: " << line << "\n    Code: " << error << "\n";
}