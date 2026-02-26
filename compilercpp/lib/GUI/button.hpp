#ifndef COMPILERCPP_LIB_GUI_BUTTON
#define COMPILERCPP_LIB_GUI_BUTTON

#include <functional>

#include "object.hpp"
#include "rect_border.hpp"
#include "text.hpp"

#include "../other/stu.hpp"
#include "../GUI_primitive/engine_def.hpp"

class button_t: public GUI_object {
private:
    engine_t* m_engine{};
    pos_2D m_pos{};
    size_2D m_size{};
    size_1D m_border_size{};
    std::wstring m_display_text{};
    depth_range_t m_depth_range{};
    rect_border_t m_rect{};
    text_t m_text{};
    std::function<void(pos_2D)>* m_mouse_move_callback{};
    std::function<void(void)>* m_mouse_leave_callback{};
    std::function<void(pos_2D)>* m_mouse_left_click_callback{};
    std::function<void(pos_2D)>* m_mouse_left_release_callback{};
    std::function<void(void)> m_callback{};
    bool m_clicked_inside{};
    
    auto mouse_move_callback(pos_2D pos) -> void;
    auto mouse_leave_callback() -> void;
    auto mouse_left_click_callback(pos_2D pos) -> void;
    auto mouse_left_release_callback(pos_2D pos) -> void;
public:
    button_t() = default;
    button_t(engine_t* engine, pos_2D pos, size_2D size, size_1D border_size, std::wstring display_text, depth_range_t depth_range, std::function<void(void)> callback);

    auto show() -> void override;
    auto hide() -> void override;
};

auto button_t::mouse_move_callback(pos_2D pos) -> void {
    if (m_rect.inside(pos)) {
        if (GetAsyncKeyState(VK_LBUTTON) < 0) {
            m_clicked_inside = false;
        }
        else {
            m_rect.border_set_color(color_t{ 1.0f, 1.0f, 0.0f });
            m_text.set_color(color_t{ 1.0f, 1.0f, 0.0f });
        }
    }
    else {
        m_rect.border_set_color(color_t{ 1.0f, 1.0f, 1.0f });
        m_rect.content_set_color(color_t{ 0.0f, 0.0f, 0.0f });
        m_text.set_color(color_t{ 1.0f, 1.0f, 1.0f });
    }
    return;
}

auto button_t::mouse_leave_callback() -> void {
    m_rect.border_set_color(color_t{ 1.0f, 1.0f, 1.0f });
    m_rect.content_set_color(color_t{ 0.0f, 0.0f, 0.0f });
    m_text.set_color(color_t{ 1.0f, 1.0f, 1.0f });
    return;
}

auto button_t::mouse_left_click_callback(pos_2D pos) -> void {
    if (m_rect.inside(pos)) {
        m_clicked_inside = true;
        m_rect.border_set_color(color_t{ 1.0f, 1.0f, 1.0f });
        m_rect.content_set_color(color_t{ 1.0f, 1.0f, 1.0f });
        m_text.set_color(color_t{ 0.0f, 0.0f, 0.0f });
    }
    return;
}

auto button_t::mouse_left_release_callback(pos_2D pos) -> void {
    if (m_rect.inside(pos)) {
        m_rect.border_set_color(color_t{ 1.0f, 1.0f, 0.0f });
        m_rect.content_set_color(color_t{ 0.0f, 0.0f, 0.0f });
        m_text.set_color(color_t{ 1.0f, 1.0f, 0.0f });
        if (m_clicked_inside) {
            m_callback();
        }
    }
    return;
}

button_t::button_t(engine_t* engine, pos_2D pos, size_2D size, size_1D border_size
, std::wstring display_text, depth_range_t depth_range, std::function<void(void)> callback)
: m_engine{ engine }, m_callback{ callback }, m_rect{ engine, pos, size, border_size, depth_range }
, m_text{ m_engine, display_text, m_rect.content_get_pos(), m_rect.content_get_size(), m_rect.content_get_pos(), m_rect.content_get_size()
, size_1D{ 0x10 }, color_t{ 1.0f, 1.0f, 1.0f }, alignment_2D{ alignment_x::center, alignment_y::center } } {}

auto button_t::show() -> void {
    m_rect.show();
    m_text.show();
    m_mouse_move_callback = m_engine->add_mouse_move(std::bind(&button_t::mouse_move_callback, this, std::placeholders::_1));
    m_mouse_leave_callback = m_engine->add_mouse_leave(std::bind(&button_t::mouse_leave_callback, this));
    m_mouse_left_click_callback = m_engine->add_mouse_left_click(std::bind(&button_t::mouse_left_click_callback, this, std::placeholders::_1));
    m_mouse_left_release_callback = m_engine->add_mouse_left_release(std::bind(&button_t::mouse_left_release_callback, this, std::placeholders::_1));
    mouse_move_callback(m_engine->get_cursor_pos());
    return;
}

auto button_t::hide() -> void {
    m_text.hide();
    m_rect.hide();
    m_engine->remove_mouse_move(m_mouse_move_callback);
    m_engine->remove_mouse_leave(m_mouse_leave_callback);
    m_engine->remove_mouse_left_click(m_mouse_left_click_callback);
    m_engine->remove_mouse_left_release(m_mouse_left_release_callback);
    return;
}

#endif