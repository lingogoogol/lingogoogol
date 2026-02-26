#ifndef COMPILERCPP_LIB_GUI_TEXT_SCROLL
#define COMPILERCPP_LIB_GUI_TEXT_SCROLL

#include <string>

#include "object.hpp"
#include "rect.hpp"
#include "text.hpp"

#include "../other/stu.hpp"
#include "../GUI_primitive/engine_decl.hpp"

class text_scroll_t: public text_t {
private:
    engine_t* m_engine{};
    size_2D m_size{};
    rect_t m_scrollbar_x{};
    rect_t m_scrollbar_y{};
    std::function<void(pos_2D, size_1D)>* m_mouse_scroll{};

    constexpr static size_1D scrollbar_width{ 0x4 };
    constexpr static std::int64_t scroll_speed{ 0x10 };

    static auto scrollbar_height(std::int64_t render_size, std::int64_t clip_size) -> std::int64_t;

    auto text_scroll_callback_scroll(pos_2D cursor_pos, size_1D scroll_size) -> void;
    auto constrain_text_pos(pos_2D& pos) -> void;
    auto update_scrollbar_pos() -> void;
    auto update_scrollbar_size() -> void;
public:
    text_scroll_t() = default;
    text_scroll_t(engine_t* engine, std::wstring text, pos_2D pos, size_2D size, float depth, size_1D size_font, color_t color, alignment_2D alignment);

    auto set_text(std::wstring text) -> void;

    auto show() -> void override;
    auto hide() -> void override;
};

auto text_scroll_t::scrollbar_height(std::int64_t render_size, std::int64_t clip_size) -> std::int64_t {
    return render_size ? std::min(clip_size * clip_size / render_size, clip_size) : clip_size;
}

auto text_scroll_t::text_scroll_callback_scroll(pos_2D cursor_pos, size_1D scroll_size) -> void {
    if (inside(get_clip_pos(), get_clip_size(), cursor_pos)) {
        pos_2D text_pos{ text_t::get_text_pos() };
        text_pos.y += scroll_speed * scroll_size.x;
        constrain_text_pos(text_pos);
        set_text_pos(text_pos);
        update_scrollbar_pos();
    }
    return;
}

auto text_scroll_t::constrain_text_pos(pos_2D& pos) -> void {
    std::int64_t scroll_y{ std::max(get_render_size().y, get_clip_size().y) };
    if (pos.y > text_t::get_clip_pos().y) {
        pos.y = get_clip_pos().y;
    }
    else if (pos.y + scroll_y < get_clip_pos().y + get_clip_size().y) {
        pos.y = get_clip_pos().y + get_clip_size().y - scroll_y;
    }
    return;
}

auto text_scroll_t::update_scrollbar_pos() -> void {
    pos_2D text_pos{ get_text_pos() };
    std::int64_t scroll_y{ std::max(get_render_size().y, get_clip_size().y) };
    m_scrollbar_y.set_pos(pos_2D{ m_scrollbar_y.get_pos().x
    , get_clip_pos().y - (text_pos.y - get_clip_pos().y) * get_clip_size().y / scroll_y });
    return;
}

auto text_scroll_t::update_scrollbar_size() -> void {
    size_2D render_size{ get_render_size() };
    std::int64_t scrollbar_height_x{ scrollbar_height(render_size.x, get_clip_size().x) };
    std::int64_t scrollbar_height_y{ scrollbar_height(render_size.y, get_clip_size().y) };
    m_scrollbar_x.set_size(size_2D{ scrollbar_height_x, scrollbar_width.x });
    m_scrollbar_y.set_size(size_2D{ scrollbar_width.x, scrollbar_height_y });
    return;
}

text_scroll_t::text_scroll_t(engine_t* engine, std::wstring text, pos_2D pos, size_2D size, float depth, size_1D size_font, color_t color, alignment_2D alignment)
: m_engine{ engine }, m_size{ size } {
    size_2D text_size{ m_size.x - scrollbar_width.x, m_size.y - scrollbar_width.x };
    text_t::init(engine, text, pos, text_size, pos, text_size, size_font, color, alignment);
    m_scrollbar_x = rect_t{ engine, pos_2D{ pos.x, pos.y + m_size.y - scrollbar_width.x }, size_2D{}, depth, color };
    m_scrollbar_y = rect_t{ engine, pos_2D{ pos.x + m_size.x - scrollbar_width.x, pos.y }, size_2D{}, depth, color };
    update_scrollbar_size();
    return;
}

auto text_scroll_t::set_text(std::wstring text) -> void {
    text_t::set_text(text);
    pos_2D text_pos{ get_text_pos() };
    constrain_text_pos(text_pos);
    set_text_pos(text_pos);
    update_scrollbar_pos();
    update_scrollbar_size();
    return;
}

auto text_scroll_t::show() -> void {
    text_t::show();
    m_scrollbar_x.show();
    m_scrollbar_y.show();
    m_mouse_scroll = m_engine->add_mouse_scroll(std::bind(&text_scroll_t::text_scroll_callback_scroll, this, std::placeholders::_1, std::placeholders::_2));
    return;
}

auto text_scroll_t::hide() -> void {
    text_t::hide();
    m_scrollbar_x.hide();
    m_scrollbar_y.hide();
    m_engine->remove_mouse_scroll(m_mouse_scroll);
    return;
}

#endif