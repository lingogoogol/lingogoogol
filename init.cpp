#include <string>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <time.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "value.h"
#include "init.h"
#include "shader_f.h"
#include "window.h"
#include "debug_f.h"
#include "holder.h"

void init_setting() {
    object::setting = Setting{
        800,
        600,
        3,
        2.5f,
        1.0f,
        0.5f,
        0.001f,
        { 0.02f, 0.1f, 0.06f },
        32,
        0.5f,
        256
    };
    return;
}

void init_window() {
    glfwInit();
    object::main_window = create_window("a fun game", object::setting.start_width, object::setting.start_height, false);
    return;
}

void init_shader() {
    object::shader.text = create_shader(constant::text_vertex_shader, constant::text_fragment_shader);
    object::shader.button = create_shader(constant::button_vertex_shader, constant::button_fragment_shader);
    object::shader.cell_surface = create_shader(constant::cell_surface_vertex_shader, constant::surface_fragment_shader);
    object::shader.object_surface = create_shader(constant::object_surface_vertex_shader, constant::surface_fragment_shader);
    return;
}

void init_font() {
    FT_Library ft{};
    if (FT_Init_FreeType(&ft))
        object::logfile << "error: Failed to initialize FreeType.\n";
    if (FT_New_Face(ft, "C:\\Windows\\Fonts\\msjh.ttc", 0, &object::font_file))
        object::logfile << "error: Failed to create new FreeType face.\n";
    FT_Set_Pixel_Sizes(object::font_file, 0, object::setting.text_resolution);
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(object::shader.text);
    glUniform1i(glGetUniformLocation(object::shader.text, "tex"), 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    check_GL_error();
    return;
}

void init_data_file() {
    object::logfile.open("123.txt");
    std::time_t time{ std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };
    std::fstream data_file{ ".\\data.nop",std::ios::out | std::ios::app };
    data_file.close();
    data_file.open(".\\data.nop", std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
    int size_difference{ static_cast<int>(sizeof(std::uint_fast64_t)) - 8 };
    if (data_file.tellp() == 0) {
        data_file.seekp(0, std::ios::beg);
        data_file << constant::current_version;
    }
    else {
        Version<5> data_version{};
        data_file.seekg(0, std::ios::beg);
        data_file >> data_version;
        if (data_version < constant::last_necessary_version) {
            auto window{ new Message_window<Message_window_type::Error>{"message",U"message11"} };
            Copy_holder holder{ static_cast<Message_window_pv*>(window),
                std::function<void(Message_window_pv*)>{ std::mem_fn(&Message_window_pv::destruct) } };
            object::shader.text = create_shader(constant::text_vertex_shader, constant::text_fragment_shader);
            object::message_window.push_back(holder);
        }
    }
    char time_cstring[26]{};
    ctime_s(time_cstring, sizeof(time_cstring), &time);
    std::string date{};
    for (int i{ 20 }; i < 24; i++)
        date.push_back(time_cstring[i]);
    date.push_back('.');
    switch (time_cstring[4]) {
    case 'J':
        switch (time_cstring[5]) {
        case 'a':
            date.push_back('1');
            break;
        case 'u':
            switch (time_cstring[6]) {
            case 'n':
                date.push_back('6');
                break;
            case 'l':
                date.push_back('7');
                break;
            }
            break;
        }
        break;
    case 'F':
        date.push_back('2');
        break;
    case 'M':
        switch (time_cstring[6]) {
        case 'r':
            date.push_back('3');
            break;
        case 'y':
            date.push_back('5');
            break;
        }
        break;
    case 'A':
        switch (time_cstring[5]) {
        case 'p':
            date.push_back('4');
            break;
        case 'u':
            date.push_back('8');
            break;
        }
        break;
    case 'S':
        date.push_back('9');
        break;
    case 'O':
        date.push_back('1');
        date.push_back('0');
        break;
    case 'N':
        date.push_back('1');
        date.push_back('1');
        break;
    case 'D':
        date.push_back('1');
        date.push_back('2');
        break;
    }
    date.push_back('.');
    if (time_cstring[8] != ' ')
        date.push_back(time_cstring[8]);
    date.push_back(time_cstring[9]);
    std::string time_string{};
    for (int i{ 11 }; i < 19; i++)
        if (time_cstring[i] == ':')
            time_string.push_back('.');
        else
            time_string.push_back(time_cstring[i]);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    check_GL_error();
    return;
}