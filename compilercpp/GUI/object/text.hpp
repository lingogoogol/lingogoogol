#ifndef COMPILERCPP_GUI_OBJECT_TEXT
#define COMPILERCPP_GUI_OBJECT_TEXT

#include <string>

#include "object_def.hpp"
#include "std.hpp"

#include "../../lib/stu.hpp"
#include "../primitive/engine_def.hpp"
#include "../primitive/text_impl.hpp"

template<bool t_wrap>
class text_impl_t: public GUI_object_t {
private:
    engine_t* m_engine{};
    std::wstring m_text{};
    pos_2D m_text_pos{};
    size_2D m_text_size{};
    pos_2D m_clip_pos{};
    size_2D m_clip_size{};
    size_1D m_font_size{};
    color_t m_color{};
    alignment_2D m_alignment{};
    text_primitive_t* m_primitive{};
protected:
    auto init(engine_t* engine, pos_2D text_pos, size_2D text_size, std::wstring text
    , pos_2D clip_pos, size_2D clip_size, size_1D font_size, color_t color, alignment_2D alignment) -> void;
public:
    text_impl_t() = default;
    text_impl_t(engine_t* engine, depth_tracker_t*, pos_2D text_pos, size_2D text_size, std::wstring text
    , pos_2D clip_pos, size_2D clip_size, size_1D font_size, color_t color, alignment_2D alignment);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;
    auto depth_range_get() const -> depth_range_t;
    auto get_text() const -> const std::wstring&;
    auto set_text(std::wstring text) -> void;
    auto get_text_pos() const -> pos_2D;
    auto set_text_pos(pos_2D text_pos) -> void;
    auto get_text_size() const -> size_2D;
    auto set_text_size(size_2D text_size) -> void;
    auto get_clip_pos() const -> pos_2D;
    auto set_clip_pos(pos_2D clip_pos) -> void;
    auto get_clip_size() const -> size_2D;
    auto set_clip_size(size_2D clip_size) -> void;
    auto set_color(color_t color) -> void;
    auto set_frame_color(color_t) -> void;
    auto get_render_size() const -> size_2D;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
};

template<bool t_wrap>
auto text_impl_t<t_wrap>::init(engine_t* engine, pos_2D text_pos, size_2D text_size, std::wstring text
, pos_2D clip_pos, size_2D clip_size, size_1D font_size, color_t color, alignment_2D alignment) -> void {
    std::unique_lock lock{ m_mutex };
    m_engine = engine;
    m_text = text;
    m_text_pos = text_pos;
    m_text_size = text_size;
    m_clip_pos = clip_pos;
    m_clip_size = clip_size;
    m_font_size = font_size;
    m_color = color;
    m_alignment = alignment;
    return;
}

template<bool t_wrap>
text_impl_t<t_wrap>::text_impl_t(engine_t* engine, depth_tracker_t*, pos_2D text_pos, size_2D text_size, std::wstring text
, pos_2D clip_pos, size_2D clip_size, size_1D font_size, color_t color, alignment_2D alignment)
: m_engine{ engine }, m_text{ text }, m_text_pos{ text_pos }, m_text_size{ text_size }, m_clip_pos{ clip_pos }
, m_clip_size{ clip_size }, m_font_size{ font_size }, m_color{ color }, m_alignment{ alignment } {}

template<bool t_wrap>
auto text_impl_t<t_wrap>::get_pos() const -> pos_2D {
    return get_clip_pos();
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ m_mutex };
    size_2D offset{ pos - get_clip_pos() };
    set_clip_pos(pos);
    set_text_pos(get_text_pos() + offset);
    return;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::get_size() const -> size_2D {
    return get_clip_size();
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::get_margin() const -> size_1D {
    return size_1D{ 0 };
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::depth_range_get() const -> depth_range_t {
    return depth_range_t{};
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::get_text() const -> const std::wstring& {
    std::unique_lock lock{ m_mutex };
    return m_text;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::set_text(std::wstring text) -> void {
    std::unique_lock lock{ m_mutex };
    m_text = text;
    if (m_primitive) {
        hide();
        show();
    }
    return;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::get_text_pos() const -> pos_2D {
    std::unique_lock lock{ m_mutex };
    return m_text_pos;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::set_text_pos(pos_2D text_pos) -> void {
    std::unique_lock lock{ m_mutex };
    m_text_pos = text_pos;
    if (m_primitive) {
        m_primitive->set_text_pos(m_text_pos);
    }
    return;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::get_text_size() const -> size_2D {
    std::unique_lock lock{ m_mutex };
    return m_text_size;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::set_text_size(size_2D text_size) -> void {
    std::unique_lock lock{ m_mutex };
    m_text_size = text_size;
    if (m_primitive) {
        m_primitive->set_text_size(text_size);
    }
    return;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::get_clip_pos() const -> pos_2D {
    std::unique_lock lock{ m_mutex };
    return m_clip_pos;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::set_clip_pos(pos_2D clip_pos) -> void {
    std::unique_lock lock{ m_mutex };
    m_clip_pos = clip_pos;
    if (m_primitive) {
        m_primitive->set_clip_pos(m_clip_pos);
    }
    return;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::get_clip_size() const -> size_2D {
    std::unique_lock lock{ m_mutex };
    return m_clip_size;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::set_clip_size(size_2D clip_size) -> void {
    std::unique_lock lock{ m_mutex };
    GUI_object_t::set_size(clip_size);
    m_clip_size = clip_size;
    if (m_primitive) {
        m_primitive->set_clip_size(clip_size);
    }
    return;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::set_color(color_t color) -> void {
    std::unique_lock lock{ m_mutex };
    m_color = color;
    if (m_primitive) {
        m_primitive->set_color(m_color);
    }
    return;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::set_frame_color(color_t) -> void {}

template<bool t_wrap>
auto text_impl_t<t_wrap>::get_render_size() const -> size_2D {
    std::unique_lock lock{ m_mutex };
    if (m_primitive) {
        return m_primitive->get_render_size();
    }
    return text_primitive_t{ m_engine, m_text, m_text_pos, m_text_size, m_clip_pos, m_clip_size, m_font_size, m_color, m_alignment, t_wrap }.get_render_size();
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::show_impl(bool base) -> void {
    if (!show_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    auto engine{ m_engine };
    auto text{ m_text };
    auto text_pos{ m_text_pos };
    auto text_size{ m_text_size };
    auto clip_pos{ m_clip_pos };
    auto clip_size{ m_clip_size };
    auto font_size{ m_font_size };
    auto color{ m_color };
    auto alignment{ m_alignment };
    lock.unlock();
    auto primitive{ engine->add_text(text, text_pos, text_size, clip_pos, clip_size, font_size, color, alignment, t_wrap) };
    lock.lock();
    m_primitive = primitive;
    show_end(base);
    return;
}

template<bool t_wrap>
auto text_impl_t<t_wrap>::hide_impl(bool base) -> void {
    if (!hide_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    auto engine{ m_engine };
    auto primitive{ m_primitive };
    m_primitive = nullptr;
    lock.unlock();
    engine->remove_text(primitive);
    hide_end(base);
    return;
}

using text_line_t = text_impl_t<false>;
using text_block_t = text_impl_t<true>;

//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth, color_t color, std::wstring text, size_1D size_x
class std_text_line_t: public text_line_t {
public:
    static size_1D size_y;
    static size_1D font_size;
    static size_1D margin;

    std_text_line_t();
    std_text_line_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth, color_t color, std::wstring text, size_1D size_x);

    auto get_margin() const -> size_1D override;
};

size_1D std_text_line_t::size_y{ std_forward };
size_1D std_text_line_t::font_size{ std_font_size };
size_1D std_text_line_t::margin{ std_margin };

std_text_line_t::std_text_line_t(): std_text_line_t{ nullptr, nullptr, pos_2D{ 0, 0 }, 0.0f, std_white, L"", size_1D{ 0 } } {}

std_text_line_t::std_text_line_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth, color_t color, std::wstring text, size_1D size_x)
: text_line_t{ engine, depth_tracker, pos, size_2D{ size_x.x, size_y.x }
, text, pos, size_2D{ size_x.x, size_y.x }, font_size, color, alignment_2D{ alignment_x::left, alignment_y::center } } {}

auto std_text_line_t::get_margin() const -> size_1D {
    return margin;
}

class text_line_flex_t: public text_line_t {
private:
    size_1D m_size_x_min{};
public:
    text_line_flex_t() = default;
    text_line_flex_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float, size_1D size_x_min, size_1D size_y
    , std::wstring text, size_1D font_size, color_t color, alignment_2D alignment);

    auto set_text(std::wstring text) -> void;
};

text_line_flex_t::text_line_flex_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float, size_1D size_x_min, size_1D size_y
, std::wstring text, size_1D font_size, color_t color, alignment_2D alignment)
: text_line_t{ engine, depth_tracker, pos, size_2D{ 0, size_y.x }, L"", pos, size_2D{ 0, size_y.x }, font_size, color, alignment }, m_size_x_min{ size_x_min } {
    set_text(text);
    return;
}

auto text_line_flex_t::set_text(std::wstring text) -> void {
    std::unique_lock lock{ m_mutex };
    text_line_t::set_text(text);
    size_1D size_x{ std::max(get_render_size().x, m_size_x_min.x) };
    text_line_t::set_text_size(size_2D{ size_x.x, get_text_size().y });
    text_line_t::set_clip_size(size_2D{ size_x.x, get_clip_size().y });
    return;
}

class std_text_line_flex_t: public text_line_flex_t {
public:
    std_text_line_flex_t();
    std_text_line_flex_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth, std::wstring text, size_1D size_x_min, color_t color);
};

std_text_line_flex_t::std_text_line_flex_t(): std_text_line_flex_t{ nullptr, nullptr, pos_2D{ 0, 0 }, 0.0f, L"", size_1D{ 0 }, std_white } {}

std_text_line_flex_t::std_text_line_flex_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, float depth, std::wstring text, size_1D size_x_min, color_t color)
: text_line_flex_t{ engine, depth_tracker, pos, depth, size_x_min, std_forward, text, std_font_size, color, alignment_2D{ alignment_x::left, alignment_y::center } } {}

#endif