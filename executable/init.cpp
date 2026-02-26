#include <string>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <iostream>
#include <functional>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "value.h"
#include "init.h"
#include "shader_f.h"
#include "window.h"
#include "debug_f.h"
#include "holder.h"
#include "string_f.h"
#include "math_f.h"

void init_setting() {
    object::setting = Setting{
        800,
        600,
        3,
        2.5f,
        1.0f,
        0.5f,
        0.001f,
        { 0.5f, 0.5f, 0.5f },
        256
    };
    return;
}

void init_logfile() {
    std::filesystem::create_directory(".\\logfile\\");
    object::logfile.open(".\\logfile\\" + get_time() + ".txt");
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
    object::shader.transparent_cell = create_shader(constant::transparent_cell_vertex_shader, constant::transparent_cell_fragment_shader);
    //object::shader.transparent_object = create_shader(constant::transparent_object_vertex_shader, constant::transparent_object_fragment_shader);
    object::shader.tile_ambient = create_shader(constant::tile_vertex_shader, constant::tile_ambient_fragment_shader);
    //object::shader.tile_dir = create_shader(constant::tile_vertex_shader, constant::tile_dir_fragment_shader);
    //object::shader.tile_point = create_shader(constant::tile_vertex_shader, constant::tile_point_fragment_shader);
    //object::shader.tile_spot = create_shader(constant::tile_vertex_shader, constant::tile_spot_fragment_shader);
    //object::shader.opaque_cell = create_shader(constant::opaque_cell_vertex_shader, constant::opaque_cell_fragment_shader);
    //object::shader.opaque_object = create_shader(constant::opaque_object_vertex_shader, constant::opaque_object_fragment_shader);
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
    std::fstream data_file{ ".\\data.nop",std::ios::out | std::ios::app };
    data_file.close();
    data_file.open(".\\data.nop", std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
    if (data_file.tellp() == 0) {
        data_file << constant::current_version;
    }
    else {
        Version<5> data_version{};
        data_file.seekg(0, std::ios::beg);
        data_file >> data_version;
        if (data_version < constant::last_necessary_version) {
            Copy_holder holder{ static_cast<Message_window_pv*>(
                new Message_window<Message_window_type::Error>{"message",
                U"請先啟動版本" + to_u32string(constant::last_necessary_version.to_string())}),
                std::function<void(Message_window_pv*)>{ std::mem_fn(&Message_window_pv::destruct) } };
            object::message_window.push_back(holder);
        }
        else {
            data_file.seekg(0, std::ios::beg);
            data_file << data_version;
        }
    }
    check_GL_error();
    return;
}