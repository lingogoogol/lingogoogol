#include <functional>
#include <glad/glad.h>

#include <Library/debug_f.h>
#include <Library/graphic.h>
#include <Library/math_f.h>
#include <Library/holder.h>
#include <Library/data.h>

void handle_error1(const char* file, int line, std::u32string message) {
    *implement::data->logfile << "Error:\n" << "    File: " << file << "\n    Line: " << line << "\n    Message: " << to_string8(message.data()) << "\n";
    implement::data->message_windows->push_back(Copy_holder{ static_cast<Message_window_pv*>(
        new Message_window<Message_window_type::Error>{ "Error", message }),
        std::function<void(Message_window_pv*)>{std::bind(std::mem_fn(&Message_window_pv::destruct),
        std::placeholders::_1)} });
}

void check_GL_error1(const char* file, int line) {
    GLenum code{};
    while ((code = glGetError()) != GL_NO_ERROR) {
        *implement::data->logfile << "Error:\n" << "    File: " << file << "\n    Line: " << line << "\n    Code: " << code << "\n";
        implement::data->message_windows->push_back(Copy_holder{ static_cast<Message_window_pv*>(
            new Message_window<Message_window_type::Error>{ "Error", to_string32(std::to_string(code)) }),
            std::function<void(Message_window_pv*)>{std::bind(std::mem_fn(&Message_window_pv::destruct),
            std::placeholders::_1)} });
    }
}