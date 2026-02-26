#ifndef COMPILERCPP_LIB_GUI_RECT_BORDER
#define COMPILERCPP_LIB_GUI_RECT_BORDER

#include "object.hpp"
#include "rect.hpp"

#include "../other/stu.hpp"

class rect_border_t: public GUI_object {
private:
    pos_2D m_pos{};
    size_2D m_size{};
    size_1D m_border_size{};
    depth_range_t m_depth_range{};
    rect_t m_outer_rect{};
    rect_t m_inner_rect{};
public:
    rect_border_t() = default;
    rect_border_t(engine_t* engine, pos_2D pos, size_2D size, size_1D border_size, depth_range_t depth_range);

    auto border_set_color(color_t color) -> void;
    auto content_get_pos() -> pos_2D;
    auto content_get_size() -> size_2D;
    auto content_set_color(color_t color) -> void;

    auto inside(pos_2D pos) -> bool;

    auto show() -> void override;
    auto hide() -> void override;
};

rect_border_t::rect_border_t(engine_t* engine, pos_2D pos, size_2D size, size_1D border_size, depth_range_t depth_range)
: m_pos{ pos }, m_size{ size }, m_border_size{ border_size }, m_depth_range{ depth_range } {
    float depth_distance{ (m_depth_range.far - m_depth_range.near) / 2 };
    m_outer_rect = rect_t{ engine, m_pos, m_size, m_depth_range.far - depth_distance, color_t{ 1.0f, 1.0f, 1.0f } };
    pos_2D inner_pos{ m_pos.x + m_border_size.x, m_pos.y + m_border_size.x };
    size_2D inner_size{ m_size.x - m_border_size.x * 2, m_size.y - m_border_size.x * 2 };
    m_inner_rect = rect_t{ engine, inner_pos, inner_size, m_depth_range.far - depth_distance * 2, color_t{ 0.0f, 0.0f, 0.0f } };
    return;
}

auto rect_border_t::border_set_color(color_t color) -> void {
    m_outer_rect.set_color(color);
    return;
}

auto rect_border_t::content_get_pos() -> pos_2D {
    return m_inner_rect.get_pos();
}

auto rect_border_t::content_get_size() -> size_2D {
    return m_inner_rect.get_size();
}

auto rect_border_t::content_set_color(color_t color) -> void {
    m_inner_rect.set_color(color);
    return;
}

auto rect_border_t::inside(pos_2D pos) -> bool {
    return m_outer_rect.inside(pos);
}

auto rect_border_t::show() -> void {
    m_outer_rect.show();
    m_inner_rect.show();
    return;
}

auto rect_border_t::hide() -> void {
    m_inner_rect.hide();
    m_outer_rect.hide();
    return;
}


#endif