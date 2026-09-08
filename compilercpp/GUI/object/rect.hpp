#ifndef COMPILERCPP_GUI_OBJECT_RECT
#define COMPILERCPP_GUI_OBJECT_RECT

#include "object_def.hpp"
#include "std.hpp"

#include "../../lib/stu.hpp"
#include "../primitive/engine_def.hpp"

class rect_t: public GUI_object_t {
private:
    engine_t* m_engine{};
    depth_tracker_t* m_depth_tracker{};
    std::uint64_t m_depth_id{};
    pos_2D m_pos{};
    size_2D m_size{};
    depth_range_t m_depth_range{};
    color_t m_color{};
    const SRV_t* m_SRV{};
    pos_2D m_texture_pos{};
    size_2D m_texture_axis_x{};
    size_2D m_texture_axis_y{};
    bool m_texture_enable{};
    rect_primitive_t* m_primitive{};
    pos_2D m_clip_pos{};
    size_2D m_clip_size{};
public:
    rect_t() = default;
    rect_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_2D size);
    rect_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_2D size
    , pos_2D clip_pos, size_2D clip_size);
    rect_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_2D size, color_t color);
    rect_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_2D size, const SRV_t& SRV);
    rect_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_2D size, pos_2D clip_pos
    , size_2D clip_size, const SRV_t& SRV, pos_2D texture_pos, size_2D texture_axis_x, size_2D texture_axis_y);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;
    auto set_size(size_2D size) -> void override;
    auto set_color(color_t color) -> void;
    auto set_SRV(const SRV_t& SRV) -> void;
    auto color_enable() -> void;
    auto texture_enable() -> void;
    
    auto inside(pos_2D pos) -> bool;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
};

rect_t::rect_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_2D size)
: rect_t{ engine, depth_tracker, depth_range, pos, size, pos, size } {}

rect_t::rect_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_2D size
, pos_2D clip_pos, size_2D clip_size)
: m_engine{ engine }, m_depth_tracker{ depth_tracker }, m_pos{ pos }, m_size{ size }, m_depth_range{ depth_range }
, m_clip_pos{ clip_pos }, m_clip_size{ clip_size } {}

rect_t::rect_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_2D size, color_t color)
: rect_t{ engine, depth_tracker, depth_range, pos, size } {
    m_color = color;
    m_texture_enable = false;
    return;
}

rect_t::rect_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_2D size, const SRV_t& SRV)
: rect_t{ engine, depth_tracker, depth_range, pos, size, pos, size, SRV, pos_2D{ 0, 0 }, size_2D{ size.x, 0 }, size_2D{ 0, size.y } } {}

rect_t::rect_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_2D size, pos_2D clip_pos
, size_2D clip_size, const SRV_t& SRV, pos_2D texture_pos, size_2D texture_axis_x, size_2D texture_axis_y)
: rect_t{ engine, depth_tracker, depth_range, pos, size, clip_pos, clip_size } {
    m_SRV = &SRV;
    m_texture_pos = texture_pos;
    m_texture_axis_x = texture_axis_x;
    m_texture_axis_y = texture_axis_y;
    m_texture_enable = true;
    return;
}

auto rect_t::get_pos() const -> pos_2D {
    std::unique_lock lock{ m_mutex };
    return m_pos;
}

auto rect_t::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ m_mutex };
    m_clip_pos = m_clip_pos + (pos - m_pos);
    m_pos = pos;
    if (m_primitive) {
        m_primitive->set_pos(pos);
    }
    return;
}

auto rect_t::get_size() const -> size_2D {
    std::unique_lock lock{ m_mutex };
    return m_size;
}

auto rect_t::set_size(size_2D size) -> void {
    std::unique_lock lock{ m_mutex };
    m_size = size;
    if (m_primitive) {
        m_primitive->set_size(size);
    }
    return;
}

auto rect_t::get_margin() const -> size_1D {
    return size_1D{ 0 };
}

auto rect_t::set_color(color_t color) -> void {
    std::unique_lock lock{ m_mutex };
    m_color = color;
    if (m_primitive) {
        m_primitive->set_color(color);
    }
    return;
}

auto rect_t::set_SRV(const SRV_t& SRV) -> void {
    std::unique_lock lock{ m_mutex };
    m_SRV = &SRV;
    if (m_primitive) {
        m_primitive->set_SRV(SRV);
    }
    return;
}

auto rect_t::color_enable() -> void {
    std::unique_lock lock{ m_mutex };
    m_texture_enable = false;
    if (m_primitive) {
        m_primitive->color_enable();
    }
    return;
}

auto rect_t::texture_enable() -> void {
    std::unique_lock lock{ m_mutex };
    m_texture_enable = true;
    if (m_primitive) {
        m_primitive->texture_enable();
    }
    return;
}

auto rect_t::inside(pos_2D pos) -> bool {
    std::unique_lock lock{ m_mutex };
    return ::inside(m_pos, m_size, pos);
}

auto rect_t::show_impl(bool base) -> void {
    if (!show_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    auto engine{ m_engine };
    auto pos{ m_pos };
    auto size{ m_size };
    auto depth_range{ m_depth_range };
    auto color{ m_color };
    lock.unlock();
    rect_primitive_t* primitive{};
    if (m_texture_enable) {
        primitive = engine->add_rect(pos, size, depth_range.near, m_clip_pos, m_clip_size
        , *m_SRV, m_texture_pos, m_texture_axis_x, m_texture_axis_y, m_name + ".m_primitive");
    }
    else {
        primitive = engine->add_rect(pos, size, depth_range.near, color, m_name + ".m_primitive");
    }
    lock.lock();
    m_primitive = primitive;
    m_depth_id = m_depth_tracker->add(m_pos, m_size, m_depth_range.near, true);
    show_end(base);
    return;
}

auto rect_t::hide_impl(bool base) -> void {
    if (!hide_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    auto engine{ m_engine };
    auto primitive{ m_primitive };
    m_primitive = nullptr;
    m_depth_tracker->remove(m_depth_id);
    lock.unlock();
    engine->remove_rect(primitive);
    hide_end(base);
    return;
}

template<bool is_y>
class std_line_impl_t: public rect_t {
public:
    static size_1D width;
    static size_1D margin;
    
    std_line_impl_t();
    std_line_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_1D length, color_t color);

    auto get_margin() const -> size_1D override;
};

template<bool is_y>
size_1D std_line_impl_t<is_y>::width{ std_line_size };
template<bool is_y>
size_1D std_line_impl_t<is_y>::margin{ std_margin };

template<bool is_y>
std_line_impl_t<is_y>::std_line_impl_t(): std_line_impl_t{ nullptr, nullptr, pos_2D{ 0, 0 }, size_1D{ 0 }, 0.0f } {}

template<bool is_y>
std_line_impl_t<is_y>::std_line_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_1D length, color_t color)
: rect_t{ engine, depth_tracker, depth_range, pos, size_2D{ is_y ? width.x : length.x, is_y ? length.x : width.x }, color } {}

template<bool is_y>
auto std_line_impl_t<is_y>::get_margin() const -> size_1D {
    return margin;
}

//engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_1D length, color_t color
using std_linex_t = std_line_impl_t<false>;
//engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos, size_1D length, color_t color
using std_liney_t = std_line_impl_t<true>;

#endif
