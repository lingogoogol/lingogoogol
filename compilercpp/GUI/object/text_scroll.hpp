#ifndef COMPILERCPP_GUI_OBJECT_TEXT_SCROLL
#define COMPILERCPP_GUI_OBJECT_TEXT_SCROLL

#include <string>

#include "object_def.hpp"
#include "rect.hpp"
#include "text.hpp"
#include "click_area.hpp"

#include "../../lib/stu.hpp"
#include "../primitive/engine_decl.hpp"

template<bool t_wrap>
class text_scroll_impl_t: public text_impl_t<t_wrap> {
private:
    engine_t* m_engine{};
    size_2D m_size{};
    rect_t m_scrollbar_x{};
    rect_t m_scrollbar_y{};
    bool m_scroll_horizontal{};
    click_area_t m_click_area{};
    depth_range_t m_depth_range{};
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
    text_scroll_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth
    , std::wstring text, size_2D size, size_1D size_font, color_t color, alignment_2D alignment);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;
    auto depth_range_get() const -> depth_range_t;
    auto set_text(std::wstring text) -> void;
    auto set_frame_color(color_t color) -> void;
    auto set_scrollbar_color(color_t color) -> void;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
};

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::scrollbar_height(std::int64_t render_size, std::int64_t clip_size) -> std::int64_t {
    return render_size ? std::min(clip_size * clip_size / render_size, clip_size) : clip_size;
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::text_scroll_callback_scroll(pos_2D cursor_pos, size_1D scroll_size) -> void {
    std::unique_lock lock{ this->m_mutex };
    if (inside(this->get_clip_pos(), this->get_clip_size(), cursor_pos)) {
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
    }
    return;
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::text_scroll_callback_click_area() -> void {
    std::unique_lock lock{ this->m_mutex };
    m_scroll_horizontal = !m_scroll_horizontal;
    return;
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::constrain_text_pos(pos_2D& pos) -> void {
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

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::update_scrollbar_pos() -> void {
    std::unique_lock lock{ this->m_mutex };
    pos_2D text_pos{ this->get_text_pos() };
    std::int64_t scroll_y{ std::max(this->get_render_size().y, this->get_clip_size().y) };
    m_scrollbar_y.set_pos(pos_2D{ m_scrollbar_y.get_pos().x
    , this->get_clip_pos().y - (text_pos.y - this->get_clip_pos().y) * this->get_clip_size().y / scroll_y });
    std::int64_t scroll_x{ std::max(this->get_render_size().x, this->get_clip_size().x) };
    m_scrollbar_x.set_pos(pos_2D{ this->get_clip_pos().x - (text_pos.x - this->get_clip_pos().x) * this->get_clip_size().x / scroll_x
    , m_scrollbar_x.get_pos().y });
    return;
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::update_scrollbar_size() -> void {
    std::unique_lock lock{ this->m_mutex };
    size_2D render_size{ this->get_render_size() };
    std::int64_t scrollbar_height_x{ scrollbar_height(render_size.x, this->get_clip_size().x) };
    std::int64_t scrollbar_height_y{ scrollbar_height(render_size.y, this->get_clip_size().y) };
    m_scrollbar_x.set_size(size_2D{ scrollbar_height_x, scrollbar_width.x });
    m_scrollbar_y.set_size(size_2D{ scrollbar_width.x, scrollbar_height_y });
    return;
}

template<bool t_wrap>
text_scroll_impl_t<t_wrap>::text_scroll_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth, std::wstring text, size_2D size
, size_1D size_font, color_t color, alignment_2D alignment)
: m_engine{ engine }, m_size{ size }, m_depth_range{ depth, depth }, m_click_area{ engine, depth_tracker, pos, size, depth
, [] (pos_2D) {}, [] () {}, [] (pos_2D) {}, [] (pos_2D) {}, std::bind(&text_scroll_impl_t::text_scroll_callback_click_area, this) } {
    size_2D text_size{ m_size.x - scrollbar_width.x, m_size.y - scrollbar_width.x };
    text_impl_t<t_wrap>::init(engine, pos, text_size, text, pos, text_size, size_font, color, alignment);
    m_scrollbar_x = rect_t{ engine, depth_tracker, pos_2D{ pos.x, pos.y + m_size.y - scrollbar_width.x }, size_2D{}, depth, color };
    m_scrollbar_y = rect_t{ engine, depth_tracker, pos_2D{ pos.x + m_size.x - scrollbar_width.x, pos.y }, size_2D{}, depth, color };
    update_scrollbar_size();
    return;
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::get_pos() const -> pos_2D {
    return text_impl_t<t_wrap>::get_pos();
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ this->m_mutex };
    size_2D offset{ pos - text_impl_t<t_wrap>::get_pos() };
    text_impl_t<t_wrap>::set_pos(pos);
    m_click_area.set_pos(pos);
    m_scrollbar_x.set_pos(m_scrollbar_x.get_pos() + offset);
    m_scrollbar_y.set_pos(m_scrollbar_y.get_pos() + offset);
    return;
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::get_size() const -> size_2D {
    std::unique_lock lock{ this->m_mutex };
    return m_size;
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::get_margin() const -> size_1D {
    return size_1D{ 0 };
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::depth_range_get() const -> depth_range_t {
    std::unique_lock lock{ this->m_mutex };
    return m_depth_range;
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::set_text(std::wstring text) -> void {
    std::unique_lock lock{ this->m_mutex };
    text_impl_t<t_wrap>::set_text(text);
    pos_2D text_pos{ this->get_text_pos() };
    constrain_text_pos(text_pos);
    this->set_text_pos(text_pos);
    update_scrollbar_pos();
    update_scrollbar_size();
    return;
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::set_frame_color(color_t color) -> void {
    set_scrollbar_color(color);
    return;
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::set_scrollbar_color(color_t color) -> void {
    std::unique_lock lock{ this->m_mutex };
    m_scrollbar_x.set_color(color);
    m_scrollbar_y.set_color(color);
    return;
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::show_impl(bool base) -> void {
    if (!this->show_begin(base)) { return; }
    std::unique_lock lock{ this->m_mutex };
    text_impl_t<t_wrap>::show_impl(false);
    m_click_area.show();
    m_scrollbar_x.show();
    m_scrollbar_y.show();
    m_mouse_scroll = m_engine->add_mouse_scroll(std::bind(&text_scroll_impl_t<t_wrap>::text_scroll_callback_scroll, this, std::placeholders::_1, std::placeholders::_2));
    this->show_end(base);
    return;
}

template<bool t_wrap>
auto text_scroll_impl_t<t_wrap>::hide_impl(bool base) -> void {
    if (!this->hide_begin(base)) { return; }
    std::unique_lock lock{ this->m_mutex };
    m_click_area.hide();
    m_scrollbar_x.hide();
    m_scrollbar_y.hide();
    m_engine->remove_mouse_scroll(m_mouse_scroll);
    text_impl_t<t_wrap>::hide_impl(false);
    this->hide_end(base);
    return;
}

using text_line_scroll_t = text_scroll_impl_t<false>;
using text_block_scroll_t = text_scroll_impl_t<true>;

class std_text_block_scroll_t: public text_block_scroll_t {
public:
    static size_1D font_size;
    static size_1D margin;
    static color_t color;

    std_text_block_scroll_t();
    std_text_block_scroll_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth, std::wstring text, size_2D size);

    auto get_margin() const -> size_1D override;
};

size_1D std_text_block_scroll_t::font_size{ std_font_size };
size_1D std_text_block_scroll_t::margin{ std_margin };
color_t std_text_block_scroll_t::color{ std_white };

std_text_block_scroll_t::std_text_block_scroll_t(): std_text_block_scroll_t{ nullptr, nullptr, pos_2D{ 0, 0 }, 0.0f, L"", size_2D{ 0, 0 } } {}

std_text_block_scroll_t::std_text_block_scroll_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth, std::wstring text, size_2D size)
: text_block_scroll_t{ engine, depth_tracker, pos, depth, text, size, font_size, color, alignment_2D{ alignment_x::left, alignment_y::top } } {}

auto std_text_block_scroll_t::get_margin() const -> size_1D {
    return margin;
}

class std_text_line_scroll_t: public text_line_scroll_t {
public:
    static size_1D size_y;
    static size_1D font_size;
    static size_1D margin;

    std_text_line_scroll_t();
    std_text_line_scroll_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth, color_t color, std::wstring text, size_1D size_x);

    auto get_margin() const -> size_1D override;
};

size_1D std_text_line_scroll_t::size_y{ std_forward };
size_1D std_text_line_scroll_t::font_size{ std_font_size };
size_1D std_text_line_scroll_t::margin{ std_margin };

std_text_line_scroll_t::std_text_line_scroll_t(): std_text_line_scroll_t{ nullptr, nullptr, pos_2D{ 0, 0 }, 0.0f, std_white, L"", size_1D{ 0 } } {}

std_text_line_scroll_t::std_text_line_scroll_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth, color_t color, std::wstring text, size_1D size_x)
: text_line_scroll_t{ engine, depth_tracker, pos, depth, text, size_2D{ size_x.x, size_y.x }, font_size, color, alignment_2D{ alignment_x::left, alignment_y::top } } {}

auto std_text_line_scroll_t::get_margin() const -> size_1D {
    return margin;
}

#endif