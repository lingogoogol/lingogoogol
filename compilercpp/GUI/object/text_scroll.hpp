#ifndef COMPILERCPP_GUI_OBJECT_TEXT_SCROLL
#define COMPILERCPP_GUI_OBJECT_TEXT_SCROLL

#include <string>

#include "object_def.hpp"
#include "rect.hpp"
#include "text.hpp"
#include "click_area.hpp"

#include "../../lib/stu.hpp"
#include "../primitive/engine_decl.hpp"

//engine_t* engine,
//depth_tracker_t* depth_tracker,
//depth_range_t depth_range,
//pos_2D pos,
//std::string text,
//size_2D size,
//color_t frame_color,
//t_arg&&... arg
template<typename t_text>
class text_scroll_impl_t: public t_text {
private:
    size_2D m_size{};
    rect_t m_scrollbar_x{};
    rect_t m_scrollbar_y{};
    bool m_scroll_horizontal{};
    click_area_t m_click_area{};
    std::function<void(pos_2D, size_1D)>* m_mouse_scroll{};

    constexpr static size_1D scrollbar_width{ 0x2 };
    constexpr static std::int64_t scroll_speed{ 0x10 };

    static auto scrollbar_height(std::int64_t render_size, std::int64_t clip_size) -> std::int64_t;

    auto text_scroll_callback_scroll(pos_2D cursor_pos, size_1D scroll_size) -> void;
    auto text_scroll_callback_click_area() -> void;
    auto constrain_text_pos(pos_2D& pos) -> void;
    auto update_scrollbar_pos() -> void;
    auto update_scrollbar_size() -> void;
public:
    text_scroll_impl_t() = default;
    template<typename... t_arg>
    text_scroll_impl_t(
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        size_2D size,
        color_t frame_color,
        t_arg&&... arg
    );

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;
    auto set_text(std::string text) -> void;
    auto set_frame_color(color_t color) -> void;
    auto set_scrollbar_color(color_t color) -> void;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
};

template<typename t_text>
auto text_scroll_impl_t<t_text>::scrollbar_height(std::int64_t render_size, std::int64_t clip_size) -> std::int64_t {
    return render_size ? std::min(clip_size * clip_size / render_size, clip_size) : clip_size;
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::text_scroll_callback_scroll(pos_2D /*cursor_pos*/, size_1D /*scroll_size*/) -> void {
    std::unique_lock lock{ this->m_mutex };
    /*if (inside(this->get_clip_pos(), this->get_clip_size(), cursor_pos)) {
        pos_2D text_pos{ this->get_text_pos() };
        if (m_scroll_horizontal) {
            text_pos.x += scroll_speed * scroll_size.x;
        }
        else {
            text_pos.y += scroll_speed * scroll_size.x;
        }
        constrain_text_pos(text_pos);
        this->set_text_pos(text_pos);
        update_scrollbar_pos();
    }*/
    return;
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::text_scroll_callback_click_area() -> void {
    std::unique_lock lock{ this->m_mutex };
    m_scroll_horizontal = !m_scroll_horizontal;
    return;
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::constrain_text_pos(pos_2D& pos) -> void {
    std::unique_lock lock{ this->m_mutex };
    std::int64_t scroll_y{ std::max(this->get_render_size().y, this->get_clip_size().y) };
    if (pos.y > this->get_clip_pos().y) {
        pos.y = this->get_clip_pos().y;
    }
    else if (pos.y + scroll_y < this->get_clip_pos().y + this->get_clip_size().y) {
        pos.y = this->get_clip_pos().y + this->get_clip_size().y - scroll_y;
    }
    std::int64_t scroll_x{ std::max(this->get_render_size().x, this->get_clip_size().x) };
    if (pos.x > this->get_clip_pos().x) {
        pos.x = this->get_clip_pos().x;
    }
    else if (pos.x + scroll_x < this->get_clip_pos().x + this->get_clip_size().x) {
        pos.x = this->get_clip_pos().x + this->get_clip_size().x - scroll_x;
    }
    return;
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::update_scrollbar_pos() -> void {
    std::unique_lock lock{ this->m_mutex };
    pos_2D text_pos{ this->get_pos() };
    std::int64_t scroll_y{ std::max(this->get_text_size().y, this->get_clip_size().y) };
    m_scrollbar_y.set_pos(pos_2D{ m_scrollbar_y.get_pos().x
    , this->get_clip_pos().y - (text_pos.y - this->get_clip_pos().y) * this->get_clip_size().y / scroll_y });
    std::int64_t scroll_x{ std::max(this->get_text_size().x, this->get_clip_size().x) };
    m_scrollbar_x.set_pos(pos_2D{ this->get_clip_pos().x - (text_pos.x - this->get_clip_pos().x) * this->get_clip_size().x / scroll_x
    , m_scrollbar_x.get_pos().y });
    return;
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::update_scrollbar_size() -> void {
    std::unique_lock lock{ this->m_mutex };
    size_2D render_size{ this->get_text_size() };
    std::int64_t scrollbar_height_x{ scrollbar_height(render_size.x, this->get_clip_size().x) };
    std::int64_t scrollbar_height_y{ scrollbar_height(render_size.y, this->get_clip_size().y) };
    m_scrollbar_x.set_size(size_2D{ scrollbar_height_x, scrollbar_width.x });
    m_scrollbar_y.set_size(size_2D{ scrollbar_width.x, scrollbar_height_y });
    return;
}

template<typename t_text>
template<typename... t_arg>
text_scroll_impl_t<t_text>::text_scroll_impl_t(
    engine_t* engine,
    depth_tracker_t* depth_tracker,
    depth_range_t depth_range,
    pos_2D pos,
    std::string text,
    size_2D size,
    color_t frame_color,
    t_arg&&... arg
):
    m_size{ size },
    m_click_area{
        engine,
        depth_tracker,
        depth_range,
        pos,
        size,
        [] (pos_2D) {},
        [] () {},
        [] (pos_2D) {},
        [] (pos_2D) {},
        std::bind(&text_scroll_impl_t::text_scroll_callback_click_area, this)
    }
{
    size_2D text_size{ m_size.x - scrollbar_width.x, m_size.y - scrollbar_width.x };
    t_text::init(engine, depth_tracker, depth_range, pos, text, text_size, std::forward<t_arg>(arg)...);
    m_scrollbar_x = rect_t{ engine, depth_tracker, depth_range, pos_2D{ pos.x, pos.y + m_size.y - scrollbar_width.x }, size_2D{}, frame_color };
    m_scrollbar_y = rect_t{ engine, depth_tracker, depth_range, pos_2D{ pos.x + m_size.x - scrollbar_width.x, pos.y }, size_2D{}, frame_color };
    update_scrollbar_size();
    return;
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::get_pos() const -> pos_2D {
    return t_text::get_pos();
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ this->m_mutex };
    size_2D offset{ pos - t_text::get_pos() };
    t_text::set_pos(pos);
    m_click_area.set_pos(pos);
    m_scrollbar_x.set_pos(m_scrollbar_x.get_pos() + offset);
    m_scrollbar_y.set_pos(m_scrollbar_y.get_pos() + offset);
    return;
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::get_size() const -> size_2D {
    std::unique_lock lock{ this->m_mutex };
    return m_size;
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::get_margin() const -> size_1D {
    return size_1D{ 0 };
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::set_text(std::string text) -> void {
    std::unique_lock lock{ this->m_mutex };
    t_text::set_text(text);
    /*pos_2D text_pos{ this->get_text_pos() };
    constrain_text_pos(text_pos);
    this->set_text_pos(text_pos);
    update_scrollbar_pos();
    update_scrollbar_size();*/
    return;
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::set_frame_color(color_t color) -> void {
    set_scrollbar_color(color);
    return;
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::set_scrollbar_color(color_t color) -> void {
    std::unique_lock lock{ this->m_mutex };
    m_scrollbar_x.set_color(color);
    m_scrollbar_y.set_color(color);
    return;
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::show_impl(bool base) -> void {
    if (!this->show_begin(base)) { return; }
    std::unique_lock lock{ this->m_mutex };
    t_text::show_impl(false);
    m_click_area.show();
    m_scrollbar_x.show();
    m_scrollbar_y.show();
    m_mouse_scroll = t_text::m_engine->add_mouse_scroll(std::bind(&text_scroll_impl_t<t_text>::text_scroll_callback_scroll, this, std::placeholders::_1, std::placeholders::_2));
    this->show_end(base);
    return;
}

template<typename t_text>
auto text_scroll_impl_t<t_text>::hide_impl(bool base) -> void {
    if (!this->hide_begin(base)) { return; }
    std::unique_lock lock{ this->m_mutex };
    m_click_area.hide();
    m_scrollbar_x.hide();
    m_scrollbar_y.hide();
    t_text::m_engine->remove_mouse_scroll(m_mouse_scroll);
    t_text::hide_impl(false);
    this->hide_end(base);
    return;
}

using text_line_scroll_t = text_scroll_impl_t<text_line_t>;
//using text_block_scroll_t = text_scroll_impl_t<text_block_t>;

class text_line_scroll_std_t: public text_line_scroll_t {
public:
    static size_1D size_y;
    static size_1D font_size;
    static size_1D margin;

    text_line_scroll_std_t() = default;
    text_line_scroll_std_t(engine_t* engine, depth_tracker_t* depth_tracker
    , depth_range_t depth_range, pos_2D pos, std::string text, color_t color, size_1D size_x);

    auto get_margin() const -> size_1D override;
};

size_1D text_line_scroll_std_t::size_y{ std_forward };
size_1D text_line_scroll_std_t::font_size{ std_font_size };
size_1D text_line_scroll_std_t::margin{ std_margin };

text_line_scroll_std_t::text_line_scroll_std_t(engine_t* engine, depth_tracker_t* depth_tracker
, depth_range_t depth_range, pos_2D pos, std::string text, color_t color, size_1D size_x)
: text_line_scroll_t{ engine, depth_tracker, depth_range, pos, text, size_2D{ size_x.x, size_y.x }, color, margin
, font_preference_t::default_get(), color, alignment_2D{ alignment_x::left, alignment_y::top } } {}

auto text_line_scroll_std_t::get_margin() const -> size_1D {
    return margin;
}

/*class text_block_scroll_std_t: public text_block_scroll_t {
public:
    static size_1D font_size;
    static size_1D margin;
    static color_t color;

    text_block_scroll_std_t();
    text_block_scroll_std_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth, std::wstring text, size_2D size);

    auto get_margin() const -> size_1D override;
};

size_1D text_block_scroll_std_t::font_size{ std_font_size };
size_1D text_block_scroll_std_t::margin{ std_margin };
color_t text_block_scroll_std_t::color{ std_white };

text_block_scroll_std_t::text_block_scroll_std_t(): text_block_scroll_std_t{ nullptr, nullptr, pos_2D{ 0, 0 }, 0.0f, L"", size_2D{ 0, 0 } } {}

text_block_scroll_std_t::text_block_scroll_std_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth, std::wstring text, size_2D size)
: text_block_scroll_t{ engine, depth_tracker, pos, depth, text, size, font_size, color, alignment_2D{ alignment_x::left, alignment_y::top } } {}

auto text_block_scroll_std_t::get_margin() const -> size_1D {
    return margin;
}*/

#endif