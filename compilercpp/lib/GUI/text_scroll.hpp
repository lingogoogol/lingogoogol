#ifndef COMPILERCPP_LIB_GUI_TEXT_SCROLL
#define COMPILERCPP_LIB_GUI_TEXT_SCROLL

#include <string>

#include "object.hpp"
#include "rect.hpp"
#include "text.hpp"

#include "../stu.hpp"
#include "../GUI_primitive/engine_decl.hpp"

class text_scroll_t: public GUI_object {
private:
    engine_t* m_engine{};
    text_t m_text{};
    rect_t m_scrollbar_x{};
    rect_t m_scrollbar_y{};
    std::function<void(pos_2D, size_1D)>* m_mouse_scroll{};

    constexpr static size_1D scrollbar_size{ 0x4 };
    constexpr static std::int64_t scroll_speed{ 0x4 };

    auto callback_scroll(pos_2D cursor_pos, size_1D scroll_size) -> void;
public:
    text_scroll_t() = default;
    text_scroll_t(engine_t* engine, std::wstring text, pos_2D pos, size_2D size, float depth, size_1D size_font, color_t color, alignment_2D alignment);

    auto show() -> void override;
    auto hide() -> void override;
};

auto text_scroll_t::callback_scroll(pos_2D cursor_pos, size_1D scroll_size) -> void {
    if (inside(m_text.get_clip_pos(), m_text.get_clip_size(), cursor_pos)) {
        pos_2D text_pos{ m_text.get_text_pos() };
        text_pos.y += scroll_speed * scroll_size.x;
        m_text.set_text_pos(text_pos);
    }
    return;
}

text_scroll_t::text_scroll_t(engine_t* engine, std::wstring text, pos_2D pos, size_2D size, float depth, size_1D size_font, color_t color, alignment_2D alignment)
: m_engine{ engine }, m_scrollbar_x{ engine, pos_2D{ pos.x, pos.y + size.y - scrollbar_size.x }, size_2D{ size.x, scrollbar_size.x }, depth, color }
, m_scrollbar_y{ engine, pos_2D{ pos.x + size.x - scrollbar_size.x, pos.y }, size_2D{ scrollbar_size.x, size.y }, depth, color } {
    size_2D text_size{ size.x - scrollbar_size.x, size.y - scrollbar_size.x };
    m_text = text_t{ engine, text, pos, text_size, pos, text_size, size_font, color, alignment };
    return;
}

auto text_scroll_t::show() -> void {
    m_text.show();
    m_scrollbar_x.show();
    m_scrollbar_y.show();
    m_mouse_scroll = m_engine->add_mouse_scroll(std::bind(&text_scroll_t::callback_scroll, this, std::placeholders::_1, std::placeholders::_2));
    return;
}

auto text_scroll_t::hide() -> void {
    m_text.hide();
    m_scrollbar_x.hide();
    m_scrollbar_y.hide();
    m_engine->remove_mouse_scroll(m_mouse_scroll);
    return;
}

#endif