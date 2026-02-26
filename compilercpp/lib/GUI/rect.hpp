#ifndef COMPILERCPP_LIB_GUI_RECT
#define COMPILERCPP_LIB_GUI_RECT

#include "object.hpp"

#include "../other/stu.hpp"
#include "../GUI_primitive/engine_def.hpp"

class rect_t: public GUI_object {
private:
    engine_t* m_engine{};
    pos_2D m_pos{};
    size_2D m_size{};
    float m_depth{};
    color_t m_color{};
    rect_primitive_t* m_primitive{};
public:
    rect_t() = default;
    rect_t(engine_t* engine, pos_2D pos, size_2D size, float depth, color_t color);

    auto get_pos() -> pos_2D;
    auto set_pos(pos_2D pos) -> void;
    auto get_size() -> size_2D;
    auto set_size(size_2D size) -> void;
    auto set_color(color_t color) -> void;
    
    auto inside(pos_2D pos) -> bool;

    auto show() -> void override;
    auto hide() -> void override;
};

rect_t::rect_t(engine_t* engine, pos_2D pos, size_2D size, float depth, color_t color)
: m_engine{ engine }, m_pos{ pos }, m_size{ size }, m_depth{ depth }, m_color{ color } {}

auto rect_t::get_pos() -> pos_2D {
    return m_pos;
}

auto rect_t::set_pos(pos_2D pos) -> void {
    m_pos = pos;
    if (m_primitive) {
        m_primitive->set_pos(pos);
    }
    return;
}

auto rect_t::get_size() -> size_2D {
    return m_size;
}

auto rect_t::set_size(size_2D size) -> void {
    m_size = size;
    if (m_primitive) {
        m_primitive->set_size(size);
    }
    return;
}

auto rect_t::set_color(color_t color) -> void {
    m_color = color;
    if (m_primitive) {
        m_primitive->set_color(color);
    }
    return;
}

auto rect_t::inside(pos_2D pos) -> bool {
    return m_primitive->inside(pos);
}

auto rect_t::show() -> void {
    m_primitive = m_engine->add_rect(m_pos, m_size, m_depth, m_color);
    return;
}

auto rect_t::hide() -> void {
    m_engine->remove_rect(m_primitive);
    m_primitive = nullptr;
    return;
}

#endif