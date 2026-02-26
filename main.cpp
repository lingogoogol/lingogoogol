#include <iostream>
#include <functional>
#include <cmath>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "lib.h"
#include "data.h"
#include "surface.h"
#include "cell.h"
#include "block.h"
#include "tile.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glfwMakeContextCurrent(window);
    glViewport(0, 0, width, height);
}

void mouse_button_callback_home(GLFWwindow* window, int button, int action, int) {
    glfwMakeContextCurrent(window);
    Home_data* data{ static_cast<Home_data*>(glfwGetWindowUserPointer(window)) };
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        for (int i{ 0 }; i < data->button.size(); i++)
            if (data->button[i]->update_state_click(action))
                break;
}

void cursor_pos_callback_home(GLFWwindow* window, double x, double y) {
    glfwMakeContextCurrent(window);
    Home_data* data{ static_cast<Home_data*>(glfwGetWindowUserPointer(window)) };
    for (int i{ 0 }; i < data->button.size(); i++)
        if (data->button[i]->update_state_hover(static_cast<float>(x), static_cast<float>(y), data->main_window))
            break;
    return;
}

void cursor_pos_callback_world(GLFWwindow* window, double x, double y) {
    glfwMakeContextCurrent(window);
    World_data* data{ static_cast<World_data*>(glfwGetWindowUserPointer(window)) };
    if (data->first_cursor) {
        data->last_cursor_pos_x = static_cast<float>(x);
        data->last_cursor_pos_y = static_cast<float>(y);
        data->first_cursor = false;
    }
    float cursor_offset_x{ (static_cast<float>(x) - data->last_cursor_pos_x) * data->cursor_sensitivity };
    float cursor_offset_y{ (data->last_cursor_pos_y - static_cast<float>(y)) * data->cursor_sensitivity };
    data->yaw += cursor_offset_x;
    data->pitch += cursor_offset_y;
    if (data->pitch > (constant::pi_floor / 2.0f))
        data->pitch = (constant::pi_floor / 2.0f);
    if (data->pitch < -(constant::pi_floor / 2.0f))
        data->pitch = -(constant::pi_floor / 2.0f);
    data->camera_dir = glm::normalize(glm::vec3{ cos(data->yaw) * cos(data->pitch),
        sin(data->pitch), sin(data->yaw) * cos(data->pitch) });
    data->last_cursor_pos_x = static_cast<float>(x);
    data->last_cursor_pos_y = static_cast<float>(y);
    return;
}

void key_callback_world(GLFWwindow* window, int key, int, int action, int) {
    glfwMakeContextCurrent(window);
    World_data* data{ static_cast<World_data*>(glfwGetWindowUserPointer(window)) };
    switch (key) {
    case GLFW_KEY_ESCAPE:
        if (action == GLFW_PRESS)
            data->state = State::Home;
        break;
    default:
        break;
    }
    return;
}

void handle_input_world(World_data* data) {
    if (glfwGetKey(data->main_window, GLFW_KEY_A) == GLFW_PRESS)
        data->camera_speed -= glm::normalize(glm::cross(data->camera_dir, data->camera_up)) * data->acceleration * data->frame_period;
    if (glfwGetKey(data->main_window, GLFW_KEY_D) == GLFW_PRESS)
        data->camera_speed += glm::normalize(glm::cross(data->camera_dir, data->camera_up)) * data->acceleration * data->frame_period;
    if (glfwGetKey(data->main_window, GLFW_KEY_S) == GLFW_PRESS)
        data->camera_speed -= data->camera_dir * data->acceleration * data->frame_period;
    if (glfwGetKey(data->main_window, GLFW_KEY_W) == GLFW_PRESS)
        data->camera_speed += data->camera_dir * data->acceleration * data->frame_period;
    return;
}

void set_callback(GLFWkeyfun key_callback, GLFWcharfun char_callback, GLFWcursorposfun cursor_pos_callback,
    GLFWmousebuttonfun mouse_button_callback, GLFWscrollfun scroll_callback, GLFWdropfun drop_callback, Data_pv* data) {
    glfwSetKeyCallback(data->main_window, key_callback);
    glfwSetCharCallback(data->main_window, char_callback);
    glfwSetCursorPosCallback(data->main_window, cursor_pos_callback);
    glfwSetMouseButtonCallback(data->main_window, mouse_button_callback);
    glfwSetScrollCallback(data->main_window, scroll_callback);
    glfwSetDropCallback(data->main_window, drop_callback);
    return;
}

void home(Data_pv* data_global) {
    auto data{ static_cast<Home_data*>(data_global) };
    data->setup();
    glfwMakeContextCurrent(data->main_window);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    set_callback(nullptr, nullptr, cursor_pos_callback_home,
        mouse_button_callback_home, nullptr, nullptr, data);
    glfwSetInputMode(data->main_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glm::mat4 transform_mat{ glm::ortho(0.0f, static_cast<float>(data->main_window_width), static_cast<float>(data->main_window_height), 0.0f) };
    auto world_text{ new Text{U"歡迎！",64.0f,{0.0f,1.0f,1.0f},transform_mat} };
    auto environment_text{ new Text{U"更新環境評估",32.0f,{0.0f,1.0f,1.0f},transform_mat} };
    world_text->set_pos({ 400.0f,300.0f,0.5f });
    environment_text->set_pos({ 400.0f,400.0f,0.5f });
    data->text.push_back(world_text);
    data->text.push_back(environment_text);
    auto world_button{ new Button{200.0f,100.0f,transform_mat,
        glm::vec3{0.8f,0.8f,0.0f},glm::vec3{0.7f,0.7f,0.0f},glm::vec3{0.9f,0.9f,0.0f},
        [data]() {
            data->state = State::World;
        }
    } };
    auto environment_button{ new Button{100.0f,50.0f,transform_mat,
        glm::vec3{0.8f,0.8f,0.0f},glm::vec3{0.7f,0.7f,0.0f},glm::vec3{0.9f,0.9f,0.0f},
        [data]() {
            data->state = State::Environment;
        }
    } };
    world_button->set_pos(glm::vec3{ 400.0f,300.0f,0.0f });
    environment_button->set_pos(glm::vec3{ 400.0f,400.0f,0.0f });
    data->button.push_back(world_button);
    data->button.push_back(environment_button);
    check_GL_error();
    while (!glfwWindowShouldClose(data->main_window) && data->state == State::Home) {
        glfwMakeContextCurrent(data->main_window);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwPollEvents();
        for (int i{ 0 }; i < data->button.size(); i++)
            data->button[i]->render(data->shader->button);
        for (int i{ 0 }; i < data->text.size(); i++)
            data->text[i]->render(data->shader->text);
        glfwSwapBuffers(data->main_window);
        for (int i{ 0 }; i < data->message_window.size(); i++)
            (*data->message_window[i])->process(i);
        check_GL_error();
    }
    std::vector<Text*>::iterator text_iterator{ std::find(data->text.begin(), data->text.end(), environment_text) };
    delete* text_iterator;
    data->text.erase(text_iterator);
    text_iterator = std::find(data->text.begin(), data->text.end(), world_text);
    delete* text_iterator;
    data->text.erase(text_iterator);
    std::vector<Button*>::iterator button_iterator{ std::find(data->button.begin(), data->button.end(), environment_button) };
    delete* button_iterator;
    data->button.erase(button_iterator);
    button_iterator = std::find(data->button.begin(), data->button.end(), world_button);
    delete* button_iterator;
    data->button.erase(button_iterator);
    return;
}

void environment(Data_pv* data_global) {
    Data_pv* data{ data_global };
    set_callback(nullptr, nullptr, cursor_pos_callback_home,
        mouse_button_callback_home, nullptr, nullptr, data);
    auto pointer{ new char8_t*[16384]{} };
    std::int_fast64_t size_i{ 0 };
    glm::mat4 transform_mat{ glm::ortho(0.0f, static_cast<float>(
        data->main_window_width),static_cast<float>(data->main_window_height),0.0f) };
    Text size_t{ U"約0位元組",50.0f,{0.0f,0.0f,0.0f},transform_mat,};
    Text progress_t{U"",30.0f,{0.0f,0.0f,0.0f},transform_mat,};
    Text home_text{ U"取消",48.0f,{0.0f,1.0f,1.0f},transform_mat,};
    size_t.set_pos(glm::vec3{ 400.0f,300.0f,0.5f });
    progress_t.set_pos(glm::vec3{ 400.0f,350.0f,0.5f });
    home_text.set_pos({ 700.0f,550.0f,0.5f });
    auto home_button{ new Button{128.0f,64.0f,transform_mat,
        glm::vec3{0.8f,0.8f,0.0f},glm::vec3{0.7f,0.7f,0.0f},glm::vec3{0.9f,0.9f,0.0f},
        [data]() {
            data->state = State::Home;
        }
    } };
    home_button->set_pos({ 700.0f,550.0f,0.0f });
    data->button.push_back(home_button);
    int index{ 0 };
    for (int i{ 1048576 }; i > 1048575; i--) {
        try {
            while (!glfwWindowShouldClose(data->main_window) && data->state == State::Environment) {
                pointer[index] = new char8_t[i];
                size_i += i;
                index++;
                glfwMakeContextCurrent(data->main_window);
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glfwPollEvents();
                size_t.set_text(U"約" + to_string32(std::to_string(size_i)) + U"位元組");
                size_t.render(data->shader->text);
                progress_t.render(data->shader->text);
                home_text.render(data->shader->text);
                home_button->render(data->shader->button);
                glfwSwapBuffers(data->main_window);
            }
        }
        catch (std::bad_alloc&) {}
        progress_t.set_text(to_string32(to_string8(static_cast<float>(1048576 - i) /
            (1048576 - 1048575) * 100, 2)) + U"%");
    }
    for (int i{ 0 }; i < index; i++) {
        if (glfwWindowShouldClose(data->main_window))
            break;
        delete[] pointer[i];
        glfwMakeContextCurrent(data->main_window);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwPollEvents();
        if (index > 1)
            progress_t.set_text(to_string32(
                to_string8(static_cast<float>(i) / (index - 1) * 100, 2)) + U"%");
        size_t.render(data->shader->text);
        progress_t.render(data->shader->text);
        home_text.render(data->shader->text);
        home_button->render(data->shader->button);
        glfwSwapBuffers(data->main_window);
    }
    home_text.set_text(U"確定");
    while (!glfwWindowShouldClose(data->main_window) && data->state == State::Environment) {
        glfwMakeContextCurrent(data->main_window);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwPollEvents();
        size_t.render(data->shader->text);
        home_text.render(data->shader->text);
        home_button->render(data->shader->button);
        glfwSwapBuffers(data->main_window);
    }
    std::vector<Button*>::iterator button_iterator{ std::find(data->button.begin(), data->button.end(), home_button) };
    delete *button_iterator;
    data->button.erase(button_iterator);
    return;
}

void world(Data_pv* data_global) {
    auto data{ static_cast<World_data*>(data_global) };
    data->setup();
    for (int i{ 0 }; i < constant::tile_num; i++)
        for (int j{ 0 }; j < constant::tile_num; j++)
            data->tiles[i][j] = new Tile{};
    for (int i{ 0 }; i < constant::block_num; i++) {
        data->blocks.push_back(std::vector<std::vector<Copy_holder<Block*>>>{});
        for (int j{ 0 }; j < constant::block_num; j++) {
            data->blocks[i].push_back(std::vector<Copy_holder<Block*>>{});
            for (int k{ 0 }; k < constant::block_num; k++) {
                data->blocks[i][j].push_back(Copy_holder{ new Block{glm::ivec3{0,0,0}},
                    std::function<void(Block*)>(std::mem_fn(&Block::destruct)) });
                (*data->blocks[i][j][k])->update(data);
            }
        }
    }
    glfwMakeContextCurrent(data->main_window);
    set_callback(key_callback_world, nullptr, cursor_pos_callback_world,
        nullptr, nullptr, nullptr, data);
    glfwSetInputMode(data->main_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    data->projection = glm::perspective(data->angle_of_view,
        (float)data->main_window_width / (float)data->main_window_height, 0.1f, 100.0f);
    check_GL_error();
    unsigned int depth{ create_tex((float*)nullptr, GL_DEPTH_COMPONENT, data->main_window_width, data->main_window_height) };
    unsigned int FBO{ create_FBO(data->pos_shininess, data->normal_specular_strength, data->color, data->parent, depth) };
    while (!glfwWindowShouldClose(data->main_window) && data->state == State::World) {
        glfwMakeContextCurrent(data->main_window);
        glfwPollEvents();
        data->current_time = static_cast<float>(glfwGetTime());
        data->frame_period = data->current_time - data->last_time;
        data->last_time = data->current_time;
        handle_input_world(data);
        float speed{ glm::length(data->camera_speed) };
        float deceleration{ data->friction * data->frame_period };
        if (speed > deceleration)
            data->camera_speed *= (speed - deceleration) / speed;
        else if (speed <= deceleration)
            data->camera_speed = glm::vec3(0, 0, 0);
        data->camera_pos += data->camera_speed * data->frame_period;
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        data->view = glm::lookAt(data->camera_pos, data->camera_pos + data->camera_dir, data->camera_up);
        for (int i{ 0 }; i < constant::block_num; i++)
            for (int j{ 0 }; j < constant::block_num; j++)
                for (int k{ 0 }; k < constant::block_num; k++)
                    (*data->blocks[i][j][k])->render(data);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (int i{ 0 }; i < constant::tile_num; i++)
            for (int j{ 0 }; j < constant::tile_num; j++)
                data->tiles[i][j]->render(data);
        glfwSwapBuffers(data->main_window);
        for (int i{ 0 }; i < data->message_window.size(); i++)
            (*data->message_window[i])->process(i);
        check_GL_error();
    }
    return;
}

int main() {
    auto data_init{ new World_data{} };
    auto data{ static_cast<Data_pv*>(data_init) };
    data->state = State::Home;
    while (!glfwWindowShouldClose(data->main_window)) {
        switch (data->state) {
        case State::Home:
            home(data);
            break;
        case State::Environment:
            environment(data);
            break;
        case State::World:
            world(data);
            break;
        default:
            handle_error(U"未知的狀態。");
            break;
        }
    }
    glfwTerminate();
    return 0;
}