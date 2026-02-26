#ifndef COMPILERCPP_LIB_GUI_BUTTON_IMPL
#define COMPILERCPP_LIB_GUI_BUTTON_IMPL

#include "button_def.hpp"
#include "engine_def.hpp"

auto button_t::mouse_move_callback(pos_2D pos) -> void {
    if (m_outer_rect->inside(pos)) {
        if (GetAsyncKeyState(VK_LBUTTON) < 0) {
            m_clicked_inside = false;
        }
        else {
            m_outer_rect->set_color(color_t{ 1.0f, 1.0f, 0.0f });
            m_text->set_color(color_t{ 1.0f, 1.0f, 0.0f });
        }
    }
    else {
        m_outer_rect->set_color(color_t{ 1.0f, 1.0f, 1.0f });
        m_inner_rect->set_color(color_t{ 0.0f, 0.0f, 0.0f });
        m_text->set_color(color_t{ 1.0f, 1.0f, 1.0f });
    }
    return;
}

auto button_t::mouse_left_click_callback(pos_2D pos) -> void {
    if (m_outer_rect->inside(pos)) {
        m_clicked_inside = true;
        m_outer_rect->set_color(color_t{ 1.0f, 1.0f, 1.0f });
        m_inner_rect->set_color(color_t{ 1.0f, 1.0f, 1.0f });
        m_text->set_color(color_t{ 0.0f, 0.0f, 0.0f });
    }
    return;
}

auto button_t::mouse_left_release_callback(pos_2D pos) -> void {
    if (m_outer_rect->inside(pos)) {
        m_outer_rect->set_color(color_t{ 1.0f, 1.0f, 0.0f });
        m_inner_rect->set_color(color_t{ 0.0f, 0.0f, 0.0f });
        m_text->set_color(color_t{ 1.0f, 1.0f, 0.0f });
        if (m_clicked_inside) {
            m_callback();
        }
    }
    return;
}

button_t::button_t(engine_t* engine, pos_2D pos, size_2D size, size_1D border_size
, std::wstring display_text, depth_range_t depth_range, std::function<void(void)> callback)
: m_engine{ engine }, m_pos{ pos }, m_size{ size }, m_border_size{ border_size }, m_display_text{ display_text }, m_depth_range{ depth_range }, m_callback{ callback } {}

auto button_t::show() -> void {
    float depth_distance{ (m_depth_range.far - m_depth_range.near) / 2 };
    m_outer_rect = m_engine->add_rect(m_pos, m_size, color_t{ 1.0f, 1.0f, 1.0f }, m_depth_range.far - depth_distance);
    pos_2D inner_pos{ m_pos.x + m_border_size.x, m_pos.y + m_border_size.x };
    size_2D inner_size{ m_size.x - m_border_size.x * 2, m_size.y - m_border_size.x * 2 };
    m_inner_rect = m_engine->add_rect(inner_pos, inner_size, color_t{ 0.0f, 0.0f, 0.0f }, m_depth_range.far - depth_distance * 2);
    m_text = m_engine->add_text(m_display_text, inner_pos, inner_size, size_1D{ 0x10 }
    , color_t{ 1.0f, 1.0f, 1.0f }, alignment_2D{ alignment_x::center, alignment_y::center });
    m_mouse_move_callback = m_engine->add_mouse_move(std::bind(&button_t::mouse_move_callback, this, std::placeholders::_1));
    m_mouse_left_click_callback = m_engine->add_mouse_left_click(std::bind(&button_t::mouse_left_click_callback, this, std::placeholders::_1));
    m_mouse_left_release_callback = m_engine->add_mouse_left_release(std::bind(&button_t::mouse_left_release_callback, this, std::placeholders::_1));
    return;
}

auto button_t::hide() -> void {
    m_engine->remove_text(m_text);
    m_engine->remove_rect(m_inner_rect);
    m_engine->remove_rect(m_outer_rect);
    m_engine->remove_mouse_move(m_mouse_move_callback);
    m_engine->remove_mouse_left_click(m_mouse_left_click_callback);
    m_engine->remove_mouse_left_release(m_mouse_left_release_callback);
    return;
}

#endif