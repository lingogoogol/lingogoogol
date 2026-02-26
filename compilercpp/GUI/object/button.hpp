#ifndef COMPILERCPP_GUI_OBJECT_BUTTON
#define COMPILERCPP_GUI_OBJECT_BUTTON

#include <type_traits>

#include "object_def.hpp"
#include "rect_border.hpp"
#include "text.hpp"
#include "click_area.hpp"
#include "std.hpp"

#include "../../lib/stu.hpp"
#include "../primitive/engine_def.hpp"

template<typename t_type>
class button_template_t: public GUI_object_t {
private:
    engine_t* m_engine{};
    click_area_t m_click_area{};
    t_type* m_object{};
    
    auto button_template_mouse_move_callback(pos_2D pos) -> void;
    auto button_template_mouse_leave_callback() -> void;
    auto button_template_mouse_left_click_callback(pos_2D pos) -> void;
    auto button_template_mouse_left_release_callback(pos_2D pos) -> void;
public:
    button_template_t() = default;
    button_template_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, std::function<void(void)> callback, t_type* object);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
};

template<typename t_type>
auto button_template_t<t_type>::button_template_mouse_move_callback(pos_2D pos) -> void {
    m_object->button_move(pos);
    return;
}

template<typename t_type>
auto button_template_t<t_type>::button_template_mouse_leave_callback() -> void {
    m_object->button_leave();
    return;
}

template<typename t_type>
auto button_template_t<t_type>::button_template_mouse_left_click_callback(pos_2D pos) -> void {
    m_object->button_left_click(pos);
    return;
}

template<typename t_type>
auto button_template_t<t_type>::button_template_mouse_left_release_callback(pos_2D pos) -> void {
    m_object->button_left_release(pos);
    return;
}

template<typename t_type>
button_template_t<t_type>::button_template_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, std::function<void(void)> callback, t_type* object)
: m_engine{ engine }, m_click_area{ engine, depth_tracker, depth_range, object->get_pos(), object->get_size(), std::bind(&button_template_t<t_type>::button_template_mouse_move_callback, this, std::placeholders::_1)
, std::bind(&button_template_t<t_type>::button_template_mouse_leave_callback, this), std::bind(&button_template_t<t_type>::button_template_mouse_left_click_callback, this, std::placeholders::_1)
, std::bind(&button_template_t<t_type>::button_template_mouse_left_release_callback, this, std::placeholders::_1), callback }
, m_object{ object } {}

template<typename t_type>
auto button_template_t<t_type>::get_pos() const -> pos_2D {
    return m_click_area.get_pos();
}

template<typename t_type>
auto button_template_t<t_type>::set_pos(pos_2D pos) -> void {
    m_click_area.set_pos(pos);
    return;
}

template<typename t_type>
auto button_template_t<t_type>::get_size() const -> size_2D {
    return m_click_area.get_size();
}

template<typename t_type>
auto button_template_t<t_type>::get_margin() const -> size_1D {
    return size_1D{ 0 };
}

template<typename t_type>
auto button_template_t<t_type>::show_impl(bool base) -> void {
    if (!show_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    m_click_area.show();
    show_end(base);
    return;
}

template<typename t_type>
auto button_template_t<t_type>::hide_impl(bool base) -> void {
    if (!hide_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    m_click_area.hide();
    hide_end(base);
    return;
}

//engine_t* engine,
//depth_tracker_t* depth_tracker,
//depth_range_t depth_range,
//pos_2D pos,
//std::function<void(void)> callback,
//u_arg&&... arg
//v
//engine,
//depth_tracker,
//depth_range,
//pos,
//arg...
template<typename t_type>
class button_t: public t_type {
private:
    button_template_t<t_type> m_button{};
public:
    button_t() = default;
    template<typename... u_arg>
    button_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, std::function<void(void)> callback, u_arg&&... arg);

    auto set_pos(pos_2D pos) -> void override;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
};

template<typename t_type>
template<typename... u_arg>
button_t<t_type>::button_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, std::function<void(void)> callback, u_arg&&... arg)
: t_type{ engine, depth_tracker, depth_range, pos, std::forward<u_arg>(arg)... }, m_button{ engine, depth_tracker, depth_range, callback, static_cast<t_type*>(this) } {}

template<typename t_type>
auto button_t<t_type>::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ this->m_mutex };
    t_type::set_pos(pos);
    m_button.set_pos(pos);
    return;
}

template<typename t_type>
auto button_t<t_type>::show_impl(bool base) -> void {
    if (!this->show_begin(base)) { return; }
    std::unique_lock lock{ this->m_mutex };
    m_button.show();
    t_type::show_impl(false);
    this->show_end(base);
    return;
}

template<typename t_type>
auto button_t<t_type>::hide_impl(bool base) -> void {
    if (!this->hide_begin(base)) { return; }
    std::unique_lock lock{ this->m_mutex };
    t_type::hide_impl(false);
    m_button.hide();
    this->hide_end(base);
    return;
}

//engine_t* engine,
//depth_tracker_t* depth_tracker,
//depth_range_t depth_range,
//pos_2D pos,
//std::string text,
//color_t color,
//u_arg&&... arg
//v
//engine,
//depth_tracker,
//depth_range,
//pos,
//text,
//color,
//arg...
template<typename t_text>
class button_text_impl_t: public t_text {
private:
    color_t m_color{};
public:
    button_text_impl_t() = default;
    template<typename... u_arg>
    button_text_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, std::string text, color_t color, u_arg&&... arg);

    auto depth_range_get() const -> depth_range_t;

    auto button_move(pos_2D) -> void;
    auto button_leave() -> void;
    auto button_left_click(pos_2D) -> void;
    auto button_left_release(pos_2D) -> void;
};

template<typename t_text>
template<typename... u_arg>
button_text_impl_t<t_text>::button_text_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, std::string text, color_t color, u_arg&&... arg)
: t_text{ engine, depth_tracker, depth_range, pos, text, color, std::forward<u_arg>(arg)... }, m_color{ color } {}

template<typename t_text>
auto button_text_impl_t<t_text>::depth_range_get() const -> depth_range_t {
    return t_text::depth_range_get();
}

template<typename t_text>
auto button_text_impl_t<t_text>::button_move(pos_2D) -> void {
    t_text::set_color(std_yellow);
    return;
}

template<typename t_text>
auto button_text_impl_t<t_text>::button_leave() -> void {
    t_text::set_color(m_color);
    return;
}

template<typename t_text>
auto button_text_impl_t<t_text>::button_left_click(pos_2D) -> void {
    t_text::set_color(std_white);
    return;
}

template<typename t_text>
auto button_text_impl_t<t_text>::button_left_release(pos_2D) -> void {
    t_text::set_color(std_yellow);
    return;
}

//engine_t* engine,
//depth_tracker_t* depth_tracker,
//depth_range_t depth_range,
//pos_2D pos,
//std::function<void(void)> callback,
//std::string text,
//color_t color,
//u_arg&&... arg
//v
//engine,
//depth_tracker,
//depth_range,
//pos,
//text,
//color,
//arg...
template<typename t_text>
using button_text_t = button_t<button_text_impl_t<t_text>>;

class button_classic_impl_t: public GUI_object_t {
private:
    engine_t* m_engine{};
    rect_border_t m_rect{};
    text_line_t m_text{};
public:
    button_classic_impl_t() = default;
    button_classic_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos
    , std::string display_text, size_2D size, color_t content_color, size_1D border_size, color_t border_color);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;
    auto depth_range_get() const -> depth_range_t;
    
    auto button_move(pos_2D) -> void;
    auto button_leave() -> void;
    auto button_left_click(pos_2D) -> void;
    auto button_left_release(pos_2D) -> void;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
};

button_classic_impl_t::button_classic_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos
, std::string display_text, size_2D size, color_t content_color, size_1D border_size, color_t border_color)
: m_engine{ engine }, m_rect{ engine, depth_tracker, pos, size, content_color, border_size, border_color, depth_range }
, m_text{ m_engine, depth_tracker, depth_range, m_rect.content_get_pos(), display_text, m_rect.content_get_size()
, size_1D{ 0x10 }, font_preference_t::default_get(), std_white, alignment_2D{ alignment_x::center, alignment_y::center } } {}

auto button_classic_impl_t::get_pos() const -> pos_2D {
    return m_rect.get_pos();
}

auto button_classic_impl_t::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ m_mutex };
    m_rect.set_pos(pos);
    m_text.set_pos(m_rect.content_get_pos());
    return;
}

auto button_classic_impl_t::get_size() const -> size_2D {
    return m_rect.get_size();
}

auto button_classic_impl_t::get_margin() const -> size_1D {
    return size_1D{ 0 };
}

auto button_classic_impl_t::depth_range_get() const -> depth_range_t {
    return m_rect.depth_range_get();
}

auto button_classic_impl_t::button_move(pos_2D) -> void {
    m_rect.border_set_color(std_yellow);
    m_text.set_color(std_yellow);
    return;
}

auto button_classic_impl_t::button_leave() -> void {
    m_rect.border_set_color(std_white);
    m_rect.content_set_color(std_black);
    m_text.set_color(std_white);
    return;
}

auto button_classic_impl_t::button_left_click(pos_2D) -> void {
    m_rect.border_set_color(std_white);
    m_rect.content_set_color(std_white);
    m_text.set_color(std_black);
    return;
}

auto button_classic_impl_t::button_left_release(pos_2D) -> void {
    m_rect.border_set_color(std_yellow);
    m_rect.content_set_color(std_black);
    m_text.set_color(std_yellow);
    return;
}

auto button_classic_impl_t::show_impl(bool base) -> void {
    if (!show_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    m_rect.show();
    m_text.show();
    show_end(base);
    return;
}

auto button_classic_impl_t::hide_impl(bool base) -> void {
    if (!hide_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    m_text.hide();
    m_rect.hide();
    hide_end(base);
    return;
}

class button_classic_std_impl_t: public button_classic_impl_t {
public:
    static size_2D size;
    static color_t content_color;
    static size_1D border_size;
    static color_t border_color;
    static size_1D margin;

    button_classic_std_impl_t() = default;
    button_classic_std_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, std::string display_text);

    auto get_margin() const -> size_1D override;
};

size_2D button_classic_std_impl_t::size{ std_sideways.x, std_forward.x };
color_t button_classic_std_impl_t::content_color{ std_black };
size_1D button_classic_std_impl_t::border_size{ std_line_size };
color_t button_classic_std_impl_t::border_color{ std_white };
size_1D button_classic_std_impl_t::margin{ std_margin };

button_classic_std_impl_t::button_classic_std_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos
, std::string display_text)
: button_classic_impl_t{ engine, depth_tracker, depth_range, pos, display_text, size, content_color, size_1D{ 0x2 }, border_color } {}

auto button_classic_std_impl_t::get_margin() const -> size_1D {
    return margin;
}

//engine_t* engine,
//depth_tracker_t* depth_tracker,
//depth_range_t depth_range,
//pos_2D pos,
//std::function<void(void)> callback,
//std::string display_text,
//size_2D size,
//color_t content_color,
//size_1D border_size,
//color_t border_color
using button_classic_t = button_t<button_classic_impl_t>;
//engine_t* engine,
//depth_tracker_t* depth_tracker,
//depth_range_t depth_range,
//pos_2D pos,
//std::function<void(void)> callback,
//std::string display_text
using button_classic_std_t = button_t<button_classic_std_impl_t>;

#endif