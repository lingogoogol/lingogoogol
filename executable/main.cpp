#include <iostream>
#include <functional>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "button.h"
#include "value.h"
#include "init_f.h"
#include "window.h"
#include "text.h"
#include "debug_f.h"
#include "holder.h"
#include "surface.h"
#include "shader_f.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glfwMakeContextCurrent(window);
    glViewport(0, 0, width, height);
}

void mouse_button_callback_home(GLFWwindow* window, int button, int action, int) {
    glfwMakeContextCurrent(window);
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        for (int i{ 0 }; i < object::button.size(); i++)
            if (object::button[i]->update_state_click(action))
                break;
}

void cursor_pos_callback_home(GLFWwindow* window, double x, double y) {
    glfwMakeContextCurrent(window);
    for (int i{ 0 }; i < object::button.size(); i++)
        if (object::button[i]->update_state_hover(static_cast<float>(x), static_cast<float>(y)))
            break;
    return;
}

void cursor_pos_callback_world(GLFWwindow* window, double x, double y) {
    glfwMakeContextCurrent(window);
    if (object::first_cursor) {
        object::last_cursor_pos_x = static_cast<float>(x);
        object::last_cursor_pos_y = static_cast<float>(y);
        object::first_cursor = false;
    }
    float cursor_offset_x{ (static_cast<float>(x) - object::last_cursor_pos_x) * object::setting.cursor_sensitivity };
    float cursor_offset_y{ (object::last_cursor_pos_y - static_cast<float>(y)) * object::setting.cursor_sensitivity };
    object::yaw += cursor_offset_x;
    object::pitch += cursor_offset_y;
    if (object::pitch > (constant::pi_floor / 2.0f))
        object::pitch = (constant::pi_floor / 2.0f);
    if (object::pitch < -(constant::pi_floor / 2.0f))
        object::pitch = -(constant::pi_floor / 2.0f);
    object::camera_dir = glm::normalize(glm::vec3{ cos(object::yaw) * cos(object::pitch), sin(object::pitch), sin(object::yaw) * cos(object::pitch) });
    object::last_cursor_pos_x = static_cast<float>(x);
    object::last_cursor_pos_y = static_cast<float>(y);
    return;
}

void key_callback_world(GLFWwindow* window, int key, int, int action, int) {
    glfwMakeContextCurrent(window);
    switch (key) {
    case GLFW_KEY_ESCAPE:
        if (action == GLFW_PRESS)
            object::state = State::Home;
        break;
    default:
        break;
    }
    return;
}

void handle_input() {
    if (glfwGetKey(object::main_window, GLFW_KEY_A) == GLFW_PRESS)
        object::camera_speed -= glm::normalize(glm::cross(object::camera_dir, object::camera_up)) * object::setting.acceleration * object::frame_period;
    if (glfwGetKey(object::main_window, GLFW_KEY_D) == GLFW_PRESS)
        object::camera_speed += glm::normalize(glm::cross(object::camera_dir, object::camera_up)) * object::setting.acceleration * object::frame_period;
    if (glfwGetKey(object::main_window, GLFW_KEY_S) == GLFW_PRESS)
        object::camera_speed -= object::camera_dir * object::setting.acceleration * object::frame_period;
    if (glfwGetKey(object::main_window, GLFW_KEY_W) == GLFW_PRESS)
        object::camera_speed += object::camera_dir * object::setting.acceleration * object::frame_period;
    return;
}

void set_callback(GLFWkeyfun key_callback, GLFWcharfun char_callback, GLFWcursorposfun cursor_pos_callback,
    GLFWmousebuttonfun mouse_button_callback, GLFWscrollfun scroll_callback, GLFWdropfun drop_callback) {
    glfwSetKeyCallback(object::main_window, key_callback);
    glfwSetCharCallback(object::main_window, char_callback);
    glfwSetCursorPosCallback(object::main_window, cursor_pos_callback);
    glfwSetMouseButtonCallback(object::main_window, mouse_button_callback);
    glfwSetScrollCallback(object::main_window, scroll_callback);
    glfwSetDropCallback(object::main_window, drop_callback);
    return;
}

void home() {
    glfwMakeContextCurrent(object::main_window);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    set_callback(nullptr, nullptr, cursor_pos_callback_home,
        mouse_button_callback_home, nullptr, nullptr);
    glfwSetInputMode(object::main_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glm::mat4 transform_mat{ glm::ortho(0.0f, static_cast<float>(object::setting.start_width), static_cast<float>(object::setting.start_height), 0.0f) };
    auto text{ new Text{U"Åwªï¡I",64.0f,glm::vec3{0.0f,1.0f,1.0f},transform_mat} };
    text->set_pos(glm::vec3{ 400.0f,300.0f,0.5f });
    object::text.push_back(text);
    auto button{ new Button{200.0f,100.0f,transform_mat,
        glm::vec3{0.8f,0.8f,0.0f},glm::vec3{0.7f,0.7f,0.0f},glm::vec3{0.9f,0.9f,0.0f},
        State::World} };
    button->set_pos(glm::vec3{ 400.0f,300.0f,0.0f });
    object::button.push_back(button);
    check_GL_error();
    while (!glfwWindowShouldClose(object::main_window) && object::state == State::Home) {
        glfwMakeContextCurrent(object::main_window);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwPollEvents();
        for (int i{ 0 }; i < object::button.size(); i++)
            object::button[i]->print();
        for (int i{ 0 }; i < object::text.size(); i++)
            object::text[i]->print();
        glfwSwapBuffers(object::main_window);
        for (int i{ 0 }; i < object::message_window.size(); i++)
            (*object::message_window[i])->process(i);
        check_GL_error();
    }
    return;
}

void world() {
    glfwMakeContextCurrent(object::main_window);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    object::first_cursor = true;
    set_callback(key_callback_world, nullptr, cursor_pos_callback_world,
        nullptr, nullptr, nullptr);
    glfwSetInputMode(object::main_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    Stone_surface surface{ glm::vec3{0.0f,0.5f,1.0f},glm::vec3{-0.5f,-0.5f,1.0f},glm::vec3{0.5f,-0.5f,1.0f},
        glm::vec2{0.5f,1.0f},glm::vec2{0.0f,0.0f},glm::vec2{1.0f,0.0f} };
    surface.projection = glm::perspective(object::setting.angle_of_view,
        (float)object::setting.start_width / (float)object::setting.start_height, 0.1f, 100.0f);
    check_GL_error();

    unsigned int FBO{};
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    unsigned int pos_shininess{ apply_tex((float*)nullptr, GL_RGBA, object::setting.start_width, object::setting.start_height) };
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pos_shininess, 0);
    unsigned int normal_specular_strength{ apply_tex((float*)nullptr, GL_RGBA, object::setting.start_width, object::setting.start_height) };
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal_specular_strength, 0);
    unsigned int color{ apply_tex((float*)nullptr, GL_RGBA, object::setting.start_width, object::setting.start_height) };
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, color, 0);
    unsigned int parent{ apply_tex((int*)nullptr, GL_RGBA, object::setting.start_width, object::setting.start_height) };
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, parent, 0);
    unsigned int attachments[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, attachments);
    unsigned int depth{ apply_tex((float*)nullptr, GL_DEPTH_COMPONENT, object::setting.start_width, object::setting.start_height) };
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);

    unsigned int VAO{};
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    unsigned int VBO{};
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    float data[]{
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //unsigned int shader{ create_shader(constant::test_vertex, constant::test_fragment) };
    while (!glfwWindowShouldClose(object::main_window) && object::state == State::World) {
        glfwMakeContextCurrent(object::main_window);
        glfwPollEvents();
        object::current_time = static_cast<float>(glfwGetTime());
        object::frame_period = object::current_time - object::last_time;
        object::last_time = object::current_time;

        handle_input();

        float speed{ glm::length(object::camera_speed) };
        float deceleration{ object::setting.friction * object::frame_period };
        if (speed > deceleration)
            object::camera_speed *= (speed - deceleration) / speed;
        else if (speed <= deceleration)
            object::camera_speed = glm::vec3(0, 0, 0);
        object::camera_pos += object::camera_speed * object::frame_period;

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        surface.view = glm::lookAt(object::camera_pos, object::camera_pos + object::camera_dir, object::camera_up);
        surface.render();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(object::shader.tile_ambient);
        glBindVertexArray(VAO);
        glUniform1i(glGetUniformLocation(object::shader.tile_ambient, "color"), 0);
        glUniform3fv(glGetUniformLocation(object::shader.tile_ambient, "ambient_color"), 1, glm::value_ptr(object::setting.ambient_color));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, color);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(object::main_window);
        for (int i{ 0 }; i < object::message_window.size(); i++)
            (*object::message_window[i])->process(i);
        check_GL_error();
    }
    return;
}

int main() {
    object::state = State::Init;
    init_setting();
    init_logfile();
    init_window();
    init_shader();
    init_font();
    init_data_file();
    object::state = State::Home;
    check_GL_error();
    home();
    while (!glfwWindowShouldClose(object::main_window)) {
        switch (object::state) {
        case State::Home:
            home();
            break;
        case State::World:
            world();
            break;
        default:
            break;
        }
    }
    glfwTerminate();
    return 0;
}