#ifndef COMPILERCPP_GUI_OBJECT_PLUS
#define COMPILERCPP_GUI_OBJECT_PLUS

#include "object_def.hpp"
#include "click_area.hpp"
#include "rect_border.hpp"
#include "text.hpp"
#include "depth_tracker.hpp"

class button_plus_t: public GUI_object_t {
private:
    engine_t* m_engine{};
    click_area_t m_click_area{};
    rect_border_t m_rect{};
    rect_t m_horizontal{}, m_vertical{};
    color_t m_border_color{};
    
    auto button_plus_mouse_move_callback(pos_2D) -> void;
    auto button_plus_mouse_leave_callback() -> void;
    auto button_plus_mouse_left_click_callback(pos_2D) -> void;
    auto button_plus_mouse_left_release_callback(pos_2D) -> void;
public:
    button_plus_t() = default;
    button_plus_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, color_t content_color, size_1D border_size, color_t border_color
    , depth_range_t depth_range, std::function<void(void)> callback);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
};

auto button_plus_t::button_plus_mouse_move_callback(pos_2D) -> void {
    std::unique_lock lock{ m_mutex };
    m_rect.border_set_color(std_yellow);
    m_horizontal.set_color(std_yellow);
    m_vertical.set_color(std_yellow);
    return;
}

auto button_plus_t::button_plus_mouse_leave_callback() -> void {
    std::unique_lock lock{ m_mutex };
    m_rect.border_set_color(m_border_color);
    m_rect.content_set_color(std_black);
    m_horizontal.set_color(m_border_color);
    m_vertical.set_color(m_border_color);
    return;
}

auto button_plus_t::button_plus_mouse_left_click_callback(pos_2D) -> void {
    std::unique_lock lock{ m_mutex };
    m_rect.border_set_color(std_white);
    m_rect.content_set_color(std_white);
    m_horizontal.set_color(std_black);
    m_vertical.set_color(std_black);
    return;
}

auto button_plus_t::button_plus_mouse_left_release_callback(pos_2D) -> void {
    std::unique_lock lock{ m_mutex };
    m_rect.border_set_color(std_yellow);
    m_rect.content_set_color(std_black);
    m_horizontal.set_color(std_yellow);
    m_vertical.set_color(std_yellow);
    return;
}

button_plus_t::button_plus_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, color_t content_color, size_1D border_size, color_t border_color
, depth_range_t depth_range, std::function<void(void)> callback)
: m_engine{ engine }, m_click_area{ engine, depth_tracker, pos, size, depth_range.near
, std::bind(&button_plus_t::button_plus_mouse_move_callback, this, std::placeholders::_1)
, std::bind(&button_plus_t::button_plus_mouse_leave_callback, this), std::bind(&button_plus_t::button_plus_mouse_left_click_callback, this, std::placeholders::_1)
, std::bind(&button_plus_t::button_plus_mouse_left_release_callback, this, std::placeholders::_1), callback }
, m_rect{ engine, depth_tracker, pos, size, content_color, border_size, border_color, depth_range_t{ (depth_range.near + depth_range.far) / 2, depth_range.far } }
, m_horizontal{ engine, depth_tracker, pos_2D{ pos.x + size.x / 4, pos.y + (size.y - border_size.x) / 2 }
, size_2D{ size.x / 2, border_size.x }, depth_range.near, border_color }
, m_vertical{ engine, depth_tracker, pos_2D{ pos.x + (size.x - border_size.x) / 2, pos.y + size.y / 4 }
, size_2D{ border_size.x, size.y / 2 }, depth_range.near, border_color }, m_border_color{ border_color } {}

auto button_plus_t::get_pos() const -> pos_2D {
    return m_rect.get_pos();
}

auto button_plus_t::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ m_mutex };
    m_click_area.set_pos(pos);
    m_rect.set_pos(pos);
    m_horizontal.set_pos(pos_2D{ pos.x + get_size().x / 4, pos.y + (get_size().y - m_rect.get_border_size().x) / 2 });
    m_vertical.set_pos(pos_2D{ pos.x + (get_size().x - m_rect.get_border_size().x) / 2, pos.y + get_size().y / 4 });
    return;
}

auto button_plus_t::get_size() const -> size_2D {
    return m_rect.get_size();
}

auto button_plus_t::get_margin() const -> size_1D {
    return size_1D{ 0 };
}

auto button_plus_t::show_impl(bool base) -> void {
    if (!show_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    m_click_area.show();
    m_rect.show();
    m_horizontal.show();
    m_vertical.show();
    show_end(base);
    return;
}

auto button_plus_t::hide_impl(bool base) -> void {
    if (!hide_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    m_horizontal.hide();
    m_vertical.hide();
    m_rect.hide();
    m_click_area.hide();
    hide_end(base);
    return;
}

class std_button_plus_t: public button_plus_t {
public:
    static size_2D size;
    static color_t content_color;
    static size_1D border_size;
    static size_1D margin;

    std_button_plus_t();
    std_button_plus_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, depth_range_t depth_range, color_t color, std::function<void(void)> callback);

    auto get_margin() const -> size_1D override;
};

size_2D std_button_plus_t::size{ std_forward.x, std_forward.x };
color_t std_button_plus_t::content_color{ std_black };
size_1D std_button_plus_t::border_size{ std_line_size };
size_1D std_button_plus_t::margin{ std_margin };

std_button_plus_t::std_button_plus_t(): std_button_plus_t{ nullptr, nullptr, pos_2D{ 0x0, 0x0 }, depth_range_t{ 0.0f, 1.0f }, std_white, [] () {} } {}

std_button_plus_t::std_button_plus_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos
, depth_range_t depth_range, color_t color, std::function<void(void)> callback)
: button_plus_t{ engine, depth_tracker, pos, size, content_color, size_1D{ 0x2 }, color, depth_range, callback } {}

auto std_button_plus_t::get_margin() const -> size_1D {
    return margin;
}

#endif