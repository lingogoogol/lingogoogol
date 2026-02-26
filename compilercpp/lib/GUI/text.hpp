#ifndef COMPILERCPP_LIB_GUI_TEXT
#define COMPILERCPP_LIB_GUI_TEXT

#include <string>

#include "object.hpp"

#include "../stu.hpp"
#include "../GUI_primitive/engine_def.hpp"
#include "../GUI_primitive/text_impl.hpp"

class text_t: public GUI_object {
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
public:
    text_t() = default;
    text_t(engine_t* engine, std::wstring text, pos_2D text_pos, size_2D text_size
    , pos_2D clip_pos, size_2D clip_size, size_1D size_font, color_t color, alignment_2D alignment);

    auto get_text() -> std::wstring;
    auto set_text(std::wstring text) -> void;
    auto get_text_pos() -> pos_2D;
    auto set_text_pos(pos_2D text_pos) -> void;
    auto get_clip_pos() -> pos_2D;
    auto get_clip_size() -> size_2D;
    auto set_color(color_t color) -> void;
    auto get_render_size() -> size_2D;

    auto show() -> void override;
    auto hide() -> void override;
};

text_t::text_t(engine_t* engine, std::wstring text, pos_2D text_pos, size_2D text_size
, pos_2D clip_pos, size_2D clip_size, size_1D font_size, color_t color, alignment_2D alignment)
: m_engine{ engine }, m_text{ text }, m_text_pos{ text_pos }, m_text_size{ text_size }, m_clip_pos{ clip_pos }
, m_clip_size{ clip_size }, m_font_size{ font_size }, m_color{ color }, m_alignment{ alignment } {}

auto text_t::get_text() -> std::wstring {
    return m_text;
}

auto text_t::set_text(std::wstring text) -> void {
    m_text = text;
    if (m_primitive) {
        hide();
        show();
    }
    return;
}

auto text_t::get_text_pos() -> pos_2D {
    return m_text_pos;
}

auto text_t::set_text_pos(pos_2D text_pos) -> void {
    m_text_pos = text_pos;
    if (m_primitive) {
        hide();
        show();
    }
    return;
}

auto text_t::get_clip_pos() -> pos_2D {
    return m_clip_pos;
}

auto text_t::get_clip_size() -> size_2D {
    return m_clip_size;
}

auto text_t::set_color(color_t color) -> void {
    m_color = color;
    if (m_primitive) {
        m_primitive->set_color(m_color);
    }
    return;
}

auto text_t::get_render_size() -> size_2D {
    return text_primitive_t{ m_engine, m_text, m_text_pos, m_text_size, m_clip_pos, m_clip_size, m_font_size, m_color, m_alignment }.get_render_size();
}

auto text_t::show() -> void {
    m_primitive = m_engine->add_text(m_text, m_text_pos, m_text_size, m_clip_pos, m_clip_size, m_font_size, m_color, m_alignment);
    return;
}

auto text_t::hide() -> void {
    m_engine->remove_text(m_primitive);
    m_primitive = nullptr;
    return;
}

#endif