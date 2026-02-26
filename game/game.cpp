#include <iostream>
#include <functional>
#include <cmath>
#include <string>

#include <lgo/lib/glad.h>
#include <lgo/lib/glfw.h>
#include <lgo/line/.h>
#include <lgo/plane/.h>
#include <lgo/data/.h>

#include "data_d.h"
#include "surface_d.h"
#include "cell_d.h"
#include "block_d.h"
#include "tile_d.h"
#include "transform_f.h"

#ifndef _DEBUG
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glfwMakeContextCurrent(window);
    glViewport(0, 0, width, height);
}

void mouse_button_callback_home(GLFWwindow* window, int button, int action, int) {
    glfwMakeContextCurrent(window);
    Data* data{ get_data() };
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
        for (int i{ 0 }; i < data->button.size(); ++i)
            if (data->button[i]->update_state_click(action))
                break;
        break;
    }
}

void cursor_pos_callback_home(GLFWwindow* window, double x, double y) {
    glfwMakeContextCurrent(window);
    Data* data{ get_data() };
    for (int i{ 0 }; i < data->button.size(); ++i)
        if (data->button[i]->update_state_hover(lgo::sc<float>(x),
            lgo::sc<float>(lgo::get_window_height(window)) - lgo::sc<float>(y), window))
            return;
}

void cursor_pos_callback_world(GLFWwindow* window, double x, double y) {
    glfwMakeContextCurrent(window);
    Data* data{ get_data() };
    World_data* world_data{ get_world_data() };
    if (data->state != State::Wait) {
        if (world_data->first_cursor) {
            world_data->last_cursor_pos_x = lgo::sc<float>(x);
            world_data->last_cursor_pos_y = lgo::sc<float>(y);
            world_data->first_cursor = false;
        }
        world_data->yaw += (lgo::sc<float>(x) - world_data->last_cursor_pos_x) *
            world_data->cursor_sensitivity;
        world_data->pitch += (world_data->last_cursor_pos_y - lgo::sc<float>(y)) *
            world_data->cursor_sensitivity;
        if (world_data->pitch >= 90.0f) {
            world_data->pitch = 90.0f;
            world_data->camera_up = lgo::svec<float, 3>{ -sin(lgo::to_radians(world_data->yaw)),
                0.0f, -cos(lgo::to_radians(world_data->yaw)) };
        }
        else if (world_data->pitch <= -90.0f) {
            world_data->pitch = -90.0f;
            world_data->camera_up = lgo::svec<float, 3>{ sin(lgo::to_radians(world_data->yaw)),
                0.0f, cos(lgo::to_radians(world_data->yaw)) };
        }
        else
            world_data->camera_up = lgo::svec<float, 3>{ 0.0f, 1.0f, 0.0f };
        world_data->camera_dir = lgo::svec<float, 3>{ sin(lgo::to_radians(world_data->yaw)) *
            cos(lgo::to_radians(world_data->pitch)), sin(lgo::to_radians(world_data->pitch)),
            cos(lgo::to_radians(world_data->yaw)) * cos(lgo::to_radians(world_data->pitch)) };
    }
    world_data->last_cursor_pos_x = lgo::sc<float>(x);
    world_data->last_cursor_pos_y = lgo::sc<float>(y);
}

void key_callback_world(GLFWwindow* window, int key, int, int action, int) {
    glfwMakeContextCurrent(window);
    Data* data{ get_data() };
    World_data* world_data{ get_world_data() };
    switch (key) {
    case GLFW_KEY_ESCAPE:
        switch (action) {
        case GLFW_PRESS:
            data->state = State::Home;
            break;
        }
        break;
        //...
    case GLFW_KEY_F1:
        switch (action) {
        case GLFW_PRESS:
            for (int i{ 0 }; i < constant::cell_num; ++i)
                for (int j{ 0 }; j < constant::cell_num; ++j)
                    for (int k{ 0 }; k < constant::cell_num; ++k)
                        if (get_cell({ i,j,k })->get_type() == Cell_type::Stone)
                            std::cout << "(i == " << i << " && j == " << j << " && k == " << k << ") || \n";
            break;
        }
        break;
    case GLFW_KEY_F3:
        switch (action) {
        case GLFW_PRESS: {
            glfwSetInputMode(data->main_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            State state{ data->state };
            data->state = State::Wait;
            lgo::wait(2.0);
            data->state = state;
            glfwSetInputMode(data->main_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            world_data->first_cursor = true;
            break;
        }
        }
        break;
    }
}

void mouse_button_callback_world(GLFWwindow*, int button, int action, int) {
    World_data* world_data{ get_world_data() };
    switch (button) {
    case GLFW_MOUSE_BUTTON_RIGHT:
        switch (action) {
        case GLFW_PRESS:
            if (world_data->selected_surface)
                world_data->selected_surface->right_click();
            break;
        }
        break;
    }
}

void home() {
    Data* data{ get_data() };
    glfwMakeContextCurrent(data->main_window);
    lgo::set_callback(data->main_window, nullptr, nullptr,
        cursor_pos_callback_home, mouse_button_callback_home, nullptr);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSetInputMode(data->main_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    lgo::smat<float, 4, 4> transform_mat{ lgo::ortho(0.0f,
        lgo::sc<float>(lgo::get_window_width(data->main_window)), 0.0f,
        lgo::sc<float>(lgo::get_window_height(data->main_window)), 0.0f, 1.0f) };
    auto world_text{ new lgo::text{ U"�w��I", 64.0f, { 0.0f, 1.0f, 1.0f }, transform_mat } };
    auto environment_text{ new lgo::text{ U"���ҵ���", 32.0f, { 0.0f, 1.0f, 1.0f }, transform_mat } };
    world_text->set_pos({ 400.0f, 300.0f, 0.0f });
    environment_text->set_pos({ 700.0f, 50.0f, 0.0f });
    data->text.push_back(world_text);
    data->text.push_back(environment_text);
    auto world_button{ new lgo::button{ 200.0f, 100.0f, transform_mat,
        { 0.8f, 0.8f, 0.0f }, { 0.7f, 0.7f, 0.0f }, { 0.9f, 0.9f, 0.0f },
        [data](lgo::button&) {
            data->state = State::World;
        }
    } };
    auto environment_button{ new lgo::button{ 150.0f, 50.0f, transform_mat,
        { 0.8f, 0.8f, 0.0f }, { 0.7f, 0.7f, 0.0f }, { 0.9f, 0.9f, 0.0f },
        [data](lgo::button&) {
            data->state = State::Environment;
        }
    } };
    world_button->set_pos({ 400.0f, 300.0f, 0.0f });
    environment_button->set_pos({ 700.0f, 50.0f, 0.0f });
    data->button.push_back(world_button);
    data->button.push_back(environment_button);
    for (int i{ 0 }; i < data->button.size(); ++i)
        data->button[i]->update_state_hover(lgo::sc<float>(lgo::get_cursor_x(data->main_window)),
            lgo::sc<float>(lgo::get_cursor_y(data->main_window)), data->main_window);
    while (!glfwWindowShouldClose(data->main_window) && data->state == State::Home) {
        glfwMakeContextCurrent(data->main_window);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwPollEvents();
        for (int i{ 0 }; i < data->button.size(); ++i)
            data->button[i]->render(data->shader->button);
        for (int i{ 0 }; i < data->text.size(); ++i)
            data->text[i]->render(data->shader->text);
        glfwSwapBuffers(data->main_window);
        for (int i{ 0 }; i < data->message_window.size(); ++i)
            (*data->message_window[i])->process(i);
    }
    std::vector<lgo::text*>::iterator text_iterator{ std::find(data->text.begin(),
        data->text.end(), environment_text) };
    delete* text_iterator;
    data->text.erase(text_iterator);
    text_iterator = std::find(data->text.begin(), data->text.end(), world_text);
    delete* text_iterator;
    data->text.erase(text_iterator);
    std::vector<lgo::button*>::iterator button_iterator{ std::find(data->button.begin(),
        data->button.end(), environment_button) };
    delete* button_iterator;
    data->button.erase(button_iterator);
    button_iterator = std::find(data->button.begin(), data->button.end(), world_button);
    delete* button_iterator;
    data->button.erase(button_iterator);
}

void environment() {
    Data* data{ get_data() };
    lgo::set_callback(data->main_window, nullptr, nullptr,
        cursor_pos_callback_home, mouse_button_callback_home, nullptr);
    glDepthFunc(GL_LEQUAL);
    auto pointer{ new char8_t*[16384]{} };
    std::int_fast64_t size_i{ 0 };
    lgo::smat<float, 4, 4> transform_mat{ lgo::ortho(0.0f,
        lgo::sc<float>(lgo::get_window_width(data->main_window)), 0.0f,
        lgo::sc<float>(lgo::get_window_height(data->main_window)), 0.0f, 1.0f) };
    lgo::text size_t{ U"", 50.0f, { 0.0f, 0.0f, 0.0f }, transform_mat };
    lgo::text progress_t{ U"", 30.0f, { 0.0f, 0.0f, 0.0f }, transform_mat };
    lgo::text home_text{ U"����", 32.0f, { 0.0f, 1.0f, 1.0f }, transform_mat };
    size_t.set_pos({ 400.0f, 300.0f, 0.0f });
    progress_t.set_pos({ 400.0f, 250.0f, 0.0f });
    home_text.set_pos({ 700.0f, 50.0f, 0.0f });
    auto home_button{ new lgo::button{ 150.0f, 50.0f, transform_mat,
        { 0.8f, 0.8f, 0.0f }, { 0.7f, 0.7f, 0.0f }, { 0.9f, 0.9f, 0.0f },
        [data, &home_text](lgo::button& button) {
            home_text.set_text(U"�������K");
            std::vector<lgo::button*>::iterator i{
                std::find(data->button.begin(), data->button.end(), &button) };
            data->button.erase(i);
            State& state{ data->state };
            delete &button;
            state = State::Home;
        }
    } };
    home_button->set_pos({ 700.0f, 50.0f, 0.0f });
    data->button.push_back(home_button);
    for (int i{ 0 }; i < data->button.size(); ++i)
        data->button[i]->update_state_hover(lgo::sc<float>(lgo::get_cursor_x(data->main_window)),
            lgo::sc<float>(lgo::get_cursor_y(data->main_window)), data->main_window);
    int index{ 0 };
    for (int i{ 1048576 }; i > 1048575; --i) {
        try {
            while (!glfwWindowShouldClose(data->main_window) && data->state == State::Environment) {
                pointer[index] = new char8_t[i];
                size_i += i;
                index++;
                glfwMakeContextCurrent(data->main_window);
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glfwPollEvents();
                for (int j{ 0 }; j < data->button.size(); ++j)
                    data->button[j]->render(data->shader->button);
                size_t.set_text(U"��" + lgo::to_string32(std::to_string(size_i)) + U"�줸��");
                size_t.render(data->shader->text);
                progress_t.render(data->shader->text);
                home_text.render(data->shader->text);
                glfwSwapBuffers(data->main_window);
            }
        }
        catch (std::bad_alloc&) {}
        progress_t.set_text(lgo::to_string32(lgo::to_string8<float, 10>(lgo::sc<float>(1048576 - i) /
            (1048576 - 1048575) * 100, 2)) + U"%");
    }
    for (int i{ 0 }; i < index; ++i) {
        if (glfwWindowShouldClose(data->main_window))
            break;
        delete[] pointer[i];
        glfwMakeContextCurrent(data->main_window);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwPollEvents();
        if (index > 1)
            progress_t.set_text(lgo::to_string32(
                lgo::to_string8<float, 10>(lgo::sc<float>(i) / (index - 1) * 100, 2)) + U"%");
        for (int j{ 0 }; j < data->button.size(); ++j)
            data->button[j]->render(data->shader->button);
        size_t.render(data->shader->text);
        progress_t.render(data->shader->text);
        home_text.render(data->shader->text);
        glfwSwapBuffers(data->main_window);
    }
    home_text.set_text(U"�T�w");
    while (!glfwWindowShouldClose(data->main_window) && data->state == State::Environment) {
        glfwMakeContextCurrent(data->main_window);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwPollEvents();
        for (int i{ 0 }; i < data->button.size(); ++i)
            data->button[i]->render(data->shader->button);
        size_t.render(data->shader->text);
        home_text.render(data->shader->text);
        glfwSwapBuffers(data->main_window);
    }
}

Surface* select() {
    World_data* world_data{ get_world_data() };
    double len{ 0.0 };
    lgo::svec<float, 3> point{ world_data->camera_pos };
    while (len < world_data->select_length) {
        if (point_at_edge(point) && len != 0.0) {
            lgo::svec<int, 3> coord{ to_coord(point) };
            Cell* cell{ get_cell(coord) };
            if (!cell)
                return nullptr;
            const std::vector<Cell_surface*>& surfaces{ cell->get_include_surfaces() };
            for (int i{ 0 }; i < surfaces.size(); ++i)
                if (surfaces[i]->get_property(Surface_property::Selectable) &&
                    point_at_edge_meet_surface(point, surfaces[i])) {
                    world_data->selected_pos = point;
                    return surfaces[i];
                }
        }
        lgo::svec<int, 3> detect_coord{ to_coord(point) };
        for (int i{ 0 }; i < 3; ++i)
            if (point[i] - std::floor(point[i]) == 0.0f && world_data->camera_dir[i] < 0.0f)
                detect_coord[i] -= 1;
        Cell* cell{ get_cell(detect_coord) };
        if (!cell)
            return nullptr;
        const std::vector<Cell_surface*>& surfaces{ cell->get_include_surfaces() };
        lgo::svec<float, 3> next_point{ to_cell_side(point, world_data->camera_dir) };
        float min_length{ std::numeric_limits<float>::infinity() };
        lgo::svec<float, 3> min_intersection{};
        Surface* return_surface{};
        for (int i{ 0 }; i < surfaces.size(); ++i) {
            lgo::line line{ point, next_point };
            float current_length{ line.meet_plane(lgo::plane{surfaces[i]->get_vertices()}) };
            if (current_length == std::numeric_limits<float>::infinity() ||
                current_length == std::numeric_limits<float>::quiet_NaN())
                continue;
            lgo::svec<float, 3> intersection{ point + world_data->camera_dir * current_length };
            const lgo::arrs<lgo::svec<float, 3>, 3>& vertices{ surfaces[i]->get_vertices() };
            float dot_product1{ dot(normalize(vertices[1] - vertices[0]),
                normalize(vertices[2] - vertices[0])) };
            float dot_product2{ dot(normalize(vertices[0] - vertices[1]),
                normalize(vertices[2] - vertices[1])) };
            if (surfaces[i]->get_property(Surface_property::Selectable) &&
                current_length >= 0.0f &&
                current_length < min_length &&
                len + current_length > 0.0f &&
                len + current_length < world_data->select_length &&
                dot(normalize(intersection - vertices[0]),
                    normalize(vertices[1] - vertices[0])) >= dot_product1 &&
                dot(normalize(intersection - vertices[0]),
                    normalize(vertices[2] - vertices[0])) >= dot_product1 &&
                dot(normalize(intersection - vertices[1]),
                    normalize(vertices[0] - vertices[1])) >= dot_product2 &&
                dot(normalize(intersection - vertices[1]),
                    normalize(vertices[2] - vertices[1])) >= dot_product2) {
                min_length = current_length;
                min_intersection = intersection;
                return_surface = surfaces[i];
            }
        }
        if (return_surface) {
            world_data->selected_pos = min_intersection;
            return return_surface;
        }
        point = next_point;
        len = lgo::len(point - world_data->camera_pos);
    }
    return nullptr;
}

void world() {
    Data* data{ get_data() };
    data->state = State::World_init;
    World_data* world_data{ get_world_data() };
    for (int i{ 0 }; i < constant::tile_num; ++i)
        for (int j{ 0 }; j < constant::tile_num; ++j)
            world_data->tiles[i][j] = new Tile{};
    for (int i{ 0 }; i < constant::block_num; ++i) {
        world_data->blocks.push_back(std::vector<std::vector<Block*>>{});
        for (int j{ 0 }; j < constant::block_num; ++j) {
            world_data->blocks[i].push_back(std::vector<Block*>{});
            for (int k{ 0 }; k < constant::block_num; ++k) {
                world_data->blocks[i][j].push_back(new Block{ { i, j, k } });
                world_data->blocks[i][j][k]->gen_cell();
            }
        }
    }
    for (int i{ 0 }; i < constant::block_num; ++i)
        for (int j{ 0 }; j < constant::block_num; ++j)
            for (int k{ 0 }; k < constant::block_num; ++k)
                world_data->blocks[i][j][k]->add_placeholders();
    for (int i{ 0 }; i < constant::block_num; ++i)
        for (int j{ 0 }; j < constant::block_num; ++j)
            for (int k{ 0 }; k < constant::block_num; ++k)
                world_data->blocks[i][j][k]->update_surface();
    glfwMakeContextCurrent(data->main_window);
    lgo::set_callback(data->main_window, key_callback_world, nullptr,
        cursor_pos_callback_world, mouse_button_callback_world, nullptr);
    glDepthFunc(GL_LEQUAL);
    glfwSetInputMode(data->main_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    data->projection = lgo::perspective(world_data->angle_of_view, lgo::sc<float>(lgo::get_window_height(
        data->main_window)) / lgo::sc<float>(lgo::get_window_width(data->main_window)), 0.1f, 100.0f);
    int main_framebuffer_width{};
    int main_framebuffer_height{};
    glfwGetFramebufferSize(data->main_window, &main_framebuffer_width, &main_framebuffer_height);
    unsigned int depth{ lgo::create_tex(lgo::sc<float*>(nullptr), GL_DEPTH_COMPONENT,
        main_framebuffer_width, main_framebuffer_height) };
    unsigned int FBO{ lgo::create_fbo(world_data->pos_shininess, world_data->normal_specular_strength,
        world_data->color, world_data->parent, depth) };
    
    unsigned int VAO{};
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    unsigned int VBO{};
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 12, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, lgo::rc<void*>(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    float x0{ lgo::sc<float>(lgo::get_window_width(data->main_window)) / 2.0f };
    float y0{ lgo::sc<float>(lgo::get_window_height(data->main_window)) / 2.0f };
    float length{ y0 / 32 };
    float width{ y0 / 256 };
    float x1{ x0 - length };
    float x2{ x0 - width };
    float x3{ x0 + width };
    float x4{ x0 + length };
    float y1{ y0 - length };
    float y2{ y0 - width };
    float y3{ y0 + width };
    float y4{ y0 + length };
    float vertices[12][3]{
        { x1, y2, 0.0f },
        { x4, y2, 0.0f },
        { x1, y3, 0.0f },
        { x1, y3, 0.0f },
        { x4, y2, 0.0f },
        { x4, y3, 0.0f },
        { x2, y1, 0.0f },
        { x3, y1, 0.0f },
        { x2, y4, 0.0f },
        { x2, y4, 0.0f },
        { x3, y1, 0.0f },
        { x3, y4, 0.0f }
    };
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * 12, vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    lgo::smat<float, 4, 4> ortho_mat{ lgo::ortho(0.0f, lgo::sc<float>(lgo::get_window_width(
        data->main_window)), 0.0f, lgo::sc<float>(lgo::get_window_height(data->main_window)),
        0.0f, 1.0f) };
    lgo::text version_text{ U"�����G" + constant::current_version.to_string32(),
        20.0f, {1.0f, 1.0f, 1.0f}, ortho_mat};
    lgo::text frame_rate_text{ U"�V�v�G�p�⤤�K", 20.0f, { 1.0f, 1.0f, 1.0f }, ortho_mat };
    lgo::text pitch_text{ U"", 20.0f, { 1.0f, 1.0f, 1.0f }, ortho_mat };
    lgo::text yaw_text{ U"", 20.0f, { 1.0f, 1.0f, 1.0f }, ortho_mat };
    lgo::text camera_pos_text{ U"", 20.0f, { 1.0f, 1.0f, 1.0f }, ortho_mat };
    lgo::text camera_dir_text{ U"", 20.0f, { 1.0f, 1.0f, 1.0f }, ortho_mat };
    lgo::text camera_speed_text{ U"", 20.0f, { 1.0f, 1.0f, 1.0f }, ortho_mat };
    version_text.set_pos({ 10.0f, 585.0f, 0.0f });
    frame_rate_text.set_pos({ 10.0f, 560.0f, 0.0f });
    pitch_text.set_pos({ 10.0f, 535.0f, 0.0f });
    yaw_text.set_pos({ 10.0f, 510.0f, 0.0f });
    camera_pos_text.set_pos({ 10.0f, 485.0f, 0.0f });
    camera_dir_text.set_pos({ 10.0f, 460.0f, 0.0f });
    camera_speed_text.set_pos({ 10.0f, 435.0f, 0.0f });
    version_text.set_horizontal_alignment(lgo::dir::small);
    frame_rate_text.set_horizontal_alignment(lgo::dir::small);
    pitch_text.set_horizontal_alignment(lgo::dir::small);
    yaw_text.set_horizontal_alignment(lgo::dir::small);
    camera_pos_text.set_horizontal_alignment(lgo::dir::small);
    camera_dir_text.set_horizontal_alignment(lgo::dir::small);
    camera_speed_text.set_horizontal_alignment(lgo::dir::small);

    data->last_second = lgo::sc<int>(std::floor(glfwGetTime()));
    data->state = State::World;
    while (!glfwWindowShouldClose(data->main_window) && data->state == State::World) {
        glfwMakeContextCurrent(data->main_window);
        glfwPollEvents();
        data->current_time = lgo::sc<float>(glfwGetTime());
        data->frame_period = data->current_time - data->last_time;
        data->last_time = data->current_time;
        float speed{ lgo::len(world_data->camera_speed) };
        float deceleration{ world_data->friction * data->frame_period };
        if (speed > deceleration)
            world_data->camera_speed *= (speed - deceleration) / speed;
        else if (speed <= deceleration)
            world_data->camera_speed = { 0.0f, 0.0f, 0.0f };
        world_data->camera_pos += world_data->camera_speed * data->frame_period;

        if (glfwGetKey(data->main_window, GLFW_KEY_A) == GLFW_PRESS)
            world_data->camera_speed -= normalize(cross(world_data->camera_up,
                world_data->camera_dir)) * world_data->acceleration * data->frame_period;
        if (glfwGetKey(data->main_window, GLFW_KEY_D) == GLFW_PRESS)
            world_data->camera_speed += normalize(cross(world_data->camera_up,
                world_data->camera_dir)) * world_data->acceleration * data->frame_period;
        if (glfwGetKey(data->main_window, GLFW_KEY_S) == GLFW_PRESS)
            world_data->camera_speed -= world_data->camera_dir *
            world_data->acceleration * data->frame_period;
        if (glfwGetKey(data->main_window, GLFW_KEY_W) == GLFW_PRESS)
            world_data->camera_speed += world_data->camera_dir *
            world_data->acceleration * data->frame_period;

        Surface* selected_surface{ select() };
        if (selected_surface != world_data->selected_surface) {
            if (world_data->selected_surface)
                world_data->selected_surface->unselect();
            world_data->selected_surface = selected_surface;
            if (world_data->selected_surface)
                world_data->selected_surface->select();
        }

        if (glfwGetTime() >= lgo::sc<double>(data->last_second) + 1.0) {
            frame_rate_text.set_text(U"�V�v�G" + lgo::to_string32(std::to_string(data->frame_count)) + U"FPS");
            data->last_second++;
            data->frame_count = 0;
        }
        pitch_text.set_text(U"�������G" + lgo::to_string32<float>(world_data->pitch, 2) + U"�X");
        yaw_text.set_text(U"���\���G" + lgo::to_string32<float>(world_data->yaw, 2) + U"�X");
        camera_pos_text.set_text(U"�۾���m�G" + world_data->camera_pos.to_string32() + U"���");
        camera_dir_text.set_text(U"�۾���V�G" + world_data->camera_dir.to_string32());
        camera_speed_text.set_text(U"�۾��t�סG" + world_data->camera_speed.to_string32() + U"���/��");

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        data->view = rotate(world_data->camera_dir, world_data->camera_up) * move(world_data->camera_pos);
        for (int i{ 0 }; i < constant::block_num; ++i)
            for (int j{ 0 }; j < constant::block_num; ++j)
                for (int k{ 0 }; k < constant::block_num; ++k)
                    world_data->blocks[i][j][k]->render();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (int i{ 0 }; i < constant::tile_num; ++i)
            for (int j{ 0 }; j < constant::tile_num; ++j)
                world_data->tiles[i][j]->render();
        version_text.render(data->shader->text);
        frame_rate_text.render(data->shader->text);
        pitch_text.render(data->shader->text);
        yaw_text.render(data->shader->text);
        camera_pos_text.render(data->shader->text);
        camera_dir_text.render(data->shader->text);
        camera_speed_text.render(data->shader->text);

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glUseProgram(data->shader->button);
        lgo::svec<float, 3> color{ 1.0f,1.0f,1.0f };
        glUniform3fv(glGetUniformLocation(data->shader->button, "color"), 1, color.data());
        float* transform_mat_pointer{ make_gl_matrix(ortho_mat) };
        glUniformMatrix4fv(glGetUniformLocation(data->shader->button, "transform_mat"),
            1, GL_FALSE, transform_mat_pointer);
        lgo::free_gl_matrix(transform_mat_pointer);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 12);
        glBindVertexArray(0);

        data->frame_count++;
        glfwSwapBuffers(data->main_window);
        for (int i{ 0 }; i < data->message_window.size(); ++i)
            (*data->message_window[i])->process(i);
    }
    delete world_data;
}

int main() {
    Data* data{ get_data() };
    data->state = State::Home;
    while (!glfwWindowShouldClose(data->main_window)) {
        switch (data->state) {
        case State::Home:
            home();
            break;
        case State::Environment:
            environment();
            break;
        case State::World:
            world();
            break;
        default:
            handle_error(U"���������A�C");
            break;
        }
    }
    glfwTerminate();
    delete data;
    return 0;
}