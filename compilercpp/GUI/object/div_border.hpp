#ifndef COMPILERCPP_GUI_OBJECT_DIV_BORDER
#define COMPILERCPP_GUI_OBJECT_DIV_BORDER

#include "div_def.hpp"
#include "rect_border.hpp"

template<bool t_y, bool t_hard>
class div_border_impl_t: public GUI_div_impl_t<t_y, t_hard> {
private:
    rect_border_t m_border{};
    size_1D m_margin{};
public:
    div_border_impl_t() = default;
    div_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, color_t content_color, size_1D padding, size_1D border_size
    , color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false);
    div_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos
    , div_create_from_member, size_1D object_size_sideways, size_1D object_margin, size_1D size_forward
    , color_t content_color, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto set_size(size_2D size) -> void override;
    auto get_div_size() const -> size_2D override;
    auto set_div_size(size_2D size) -> void override;
    auto get_margin() const -> size_1D override;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
};

template<bool t_y, bool t_hard>
div_border_impl_t<t_y, t_hard>::div_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker
, pos_2D pos, size_2D size, color_t content_color, size_1D padding, size_1D border_size
, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large)
: GUI_div_impl_t<t_y, t_hard>{ engine, depth_tracker, pos + size_2D{ border_size.x, border_size.x } + size_2D{ padding.x, padding.x }
, size - size_2D{ border_size.x * 2, border_size.x * 2 } - size_2D{ padding.x * 2, padding.x * 2 }, padding, alignment, from_large }
, m_border{ engine, depth_tracker, pos, size, content_color, border_size, border_color, depth_range }, m_margin{ margin } {}

template<bool t_y, bool t_hard>
div_border_impl_t<t_y, t_hard>::div_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member
, size_1D object_size_sideways, size_1D object_margin, size_1D size_forward
, color_t content_color, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large)
: GUI_div_impl_t<t_y, t_hard>{ engine, depth_tracker, pos + size_2D{ border_size.x, border_size.x } + size_2D{ t_hard ? 0 : object_margin.x, t_hard ? 0 : object_margin.x }
, div_create_from_member{}, object_size_sideways, object_margin, size_1D{ size_forward.x - border_size.x * 2 - (t_hard ? 0 : object_margin.x * 2) }, alignment, from_large }
, m_border{ engine, depth_tracker, pos, GUI_div_impl_t<t_y, t_hard>::get_size() + size_2D{ border_size.x * 2, border_size.x * 2 }
+ size_2D{ t_hard ? 0 : object_margin.x * 2, t_hard ? 0 : object_margin.x * 2 }, content_color, border_size, border_color, depth_range }, m_margin{ margin } {}

template<bool t_y, bool t_hard>
auto div_border_impl_t<t_y, t_hard>::get_pos() const -> pos_2D {
    return m_border.get_pos();
}

template<bool t_y, bool t_hard>
auto div_border_impl_t<t_y, t_hard>::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ this->m_mutex };
    GUI_div_impl_t<t_y, t_hard>::set_pos(pos + size_2D{ m_border.get_border_size().x, m_border.get_border_size().x }
    + size_2D{ GUI_div_impl_t<t_y, t_hard>::get_margin().x, GUI_div_impl_t<t_y, t_hard>::get_margin().x });
    m_border.set_pos(pos);
    return;
}

template<bool t_y, bool t_hard>
auto div_border_impl_t<t_y, t_hard>::get_size() const -> size_2D {
    return m_border.get_size();
}

template<bool t_y, bool t_hard>
auto div_border_impl_t<t_y, t_hard>::set_size(size_2D size) -> void {
    std::unique_lock lock{ this->m_mutex };
    GUI_div_impl_t<t_y, t_hard>::set_size(size - size_2D{ m_border.get_border_size().x * 2, m_border.get_border_size().x * 2 }
    - size_2D{ GUI_div_impl_t<t_y, t_hard>::get_margin().x * 2, GUI_div_impl_t<t_y, t_hard>::get_margin().x * 2 });
    m_border.set_size(size);
    return;
}

template<bool t_y, bool t_hard>
auto div_border_impl_t<t_y, t_hard>::get_div_size() const -> size_2D {
    return GUI_div_impl_t<t_y, t_hard>::get_div_size();
}

template<bool t_y, bool t_hard>
auto div_border_impl_t<t_y, t_hard>::set_div_size(size_2D size) -> void {
    std::unique_lock lock{ this->m_mutex };
    GUI_div_impl_t<t_y, t_hard>::set_size(size);
    m_border.set_size(size + size_2D{ m_border.get_border_size().x * 2, m_border.get_border_size().x * 2 }
    + size_2D{ GUI_div_impl_t<t_y, t_hard>::get_margin().x * 2, GUI_div_impl_t<t_y, t_hard>::get_margin().x * 2 });
    return;
}

template<bool t_y, bool t_hard>
auto div_border_impl_t<t_y, t_hard>::get_margin() const -> size_1D {
    std::unique_lock lock{ this->m_mutex };
    return m_margin;
}

template<bool t_y, bool t_hard>
auto div_border_impl_t<t_y, t_hard>::show_impl(bool base) -> void {
    if (!this->show_begin(base)) { return; }
    GUI_div_impl_t<t_y, t_hard>::show_impl(false);
    m_border.show();
    this->show_end(base);
    return;
}

template<bool t_y, bool t_hard>
auto div_border_impl_t<t_y, t_hard>::hide_impl(bool base) -> void {
    if (!this->hide_begin(base)) { return; }
    m_border.hide();
    GUI_div_impl_t<t_y, t_hard>::hide_impl();
    this->hide_end(base);
    return;
}

//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, color_t content_color, size_1D padding, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, size_1D size_forward, color_t content_color, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
using divx_soft_border_t = div_border_impl_t<false, false>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, color_t content_color, size_1D padding, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, size_1D size_forward, color_t content_color, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
using divx_hard_border_t = div_border_impl_t<false, true>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, color_t content_color, size_1D padding, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, size_1D size_forward, color_t content_color, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
using divy_soft_border_t = div_border_impl_t<true, false>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, color_t content_color, size_1D padding, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, size_1D size_forward, color_t content_color, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
using divy_hard_border_t = div_border_impl_t<true, true>;

template<bool t_y>
class std_div_border_impl_t: public div_border_impl_t<t_y, true> {
public:
    static color_t content_color;
    static size_1D padding;
    static size_1D border_size;
    static size_1D margin;

    std_div_border_impl_t() = default;
    std_div_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size
    , color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large = false);
    std_div_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin
    , size_1D size_forward, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large = false);
};

template<bool t_y>
color_t std_div_border_impl_t<t_y>::content_color{ std_black };
template<bool t_y>
size_1D std_div_border_impl_t<t_y>::padding{ 0 };
template<bool t_y>
size_1D std_div_border_impl_t<t_y>::border_size{ std_line_size };
template<bool t_y>
size_1D std_div_border_impl_t<t_y>::margin{ std_margin };

template<bool t_y>
std_div_border_impl_t<t_y>::std_div_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size
, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large)
: div_border_impl_t<t_y, true>{ engine, depth_tracker, pos, size, content_color, padding, border_size, border_color, margin, alignment, depth_range, from_large } {}

template<bool t_y>
std_div_border_impl_t<t_y>::std_div_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos
, div_create_from_member, size_1D object_size_sideways, size_1D object_margin
, size_1D size_forward, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large)
: div_border_impl_t<t_y, true>{ engine, depth_tracker, pos, div_create_from_member{}, object_size_sideways, object_margin
, size_forward, content_color, border_size, border_color, margin, alignment, depth_range, from_large } {}

//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, size_1D size_forward, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
using std_divx_border_t = std_div_border_impl_t<false>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, size_1D size_forward, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
using std_divy_border_t = std_div_border_impl_t<true>;

template<bool t_y, bool t_hard>
class div_flex_border_impl_t: public GUI_div_flex_impl_t<t_y, t_hard> {
private:
    rect_border_t m_border{};
    size_1D m_margin{};
protected:
    template<typename t_object, bool t_ghost, typename... t_arg>
    auto add_object_impl(depth_range_t depth_range, t_arg&&... arg) -> std::uint64_t;

    template<bool t_y, bool t_hard>
    friend class GUI_div_impl_t;
public:
    div_flex_border_impl_t() = default;
    div_flex_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways, color_t content_color, size_1D padding
    , size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false);
    div_flex_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin
    , color_t content_color, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto set_size(size_2D size) -> void override;
    auto get_div_size() const -> size_2D override;
    auto set_div_size(size_2D size) -> void override;
    auto get_margin() const -> size_1D override;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
};

template<bool t_y, bool t_hard>
template<typename t_object, bool t_ghost, typename... t_arg>
auto div_flex_border_impl_t<t_y, t_hard>::add_object_impl(depth_range_t depth_range, t_arg&&... arg) -> std::uint64_t {
    std::unique_lock lock{ this->m_mutex };
    std::uint64_t out{ GUI_div_flex_impl_t<t_y, t_hard>::template add_object_impl<t_object, t_ghost>(depth_range, std::forward<t_arg>(arg)...) };
    m_border.set_size(GUI_div_flex_impl_t<t_y, t_hard>::get_size() + size_2D{ GUI_div_flex_impl_t<t_y, t_hard>::get_margin().x * 2
    , GUI_div_flex_impl_t<t_y, t_hard>::get_margin().x * 2 } + size_2D{ m_border.get_border_size().x * 2, m_border.get_border_size().x * 2 });
    return out;
}

template<bool t_y, bool t_hard>
div_flex_border_impl_t<t_y, t_hard>::div_flex_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways
, color_t content_color, size_1D padding, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large)
: GUI_div_flex_impl_t<t_y, t_hard>{ engine, depth_tracker, pos + size_2D{ border_size.x, border_size.x } + size_2D{ padding.x, padding.x }
, size_1D{ size_sideways.x - border_size.x * 2 - padding.x * 2 }, padding, alignment, from_large }, m_margin{ margin } {
    size_1D div_size_forward{ t_y ? GUI_div_impl_t<t_y, t_hard>::get_size().y : GUI_div_impl_t<t_y, t_hard>::get_size().x };
    size_1D size_forward{ div_size_forward.x + padding.x * 2 + border_size.x * 2 };
    m_border = rect_border_t{ engine, depth_tracker, pos, size_2D{ t_y ? size_sideways.x : size_forward.x, t_y ? size_forward.x : size_sideways.x }
    , content_color, border_size, border_color, depth_range };
    return;
}

template<bool t_y, bool t_hard>
div_flex_border_impl_t<t_y, t_hard>::div_flex_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos
, div_create_from_member, size_1D object_size_sideways, size_1D object_margin
, color_t content_color, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large)
: GUI_div_flex_impl_t<t_y, t_hard>{ engine, depth_tracker
, pos + size_2D{ border_size.x, border_size.x } + size_2D{ t_hard ? 0 : object_margin.x, t_hard ? 0 : object_margin.x }
, div_create_from_member{}, object_size_sideways, object_margin, alignment, from_large }, m_margin{ margin } {
    size_1D div_size_forward{ t_y ? GUI_div_impl_t<t_y, t_hard>::get_size().y : GUI_div_impl_t<t_y, t_hard>::get_size().x };
    size_1D size_forward{ div_size_forward + GUI_div_impl_t<t_y, t_hard>::get_margin().x * 2 + border_size.x * 2 };
    size_1D div_size_sideways{ t_y ? GUI_div_impl_t<t_y, t_hard>::get_size().x : GUI_div_impl_t<t_y, t_hard>::get_size().y };
    size_1D size_sideways{ div_size_sideways + GUI_div_impl_t<t_y, t_hard>::get_margin().x * 2 + border_size.x * 2 };
    m_border = rect_border_t{ engine, depth_tracker, pos, size_2D{ t_y ? size_sideways.x : size_forward.x, t_y ? size_forward.x : size_sideways.x }
    , content_color, border_size, border_color, depth_range };
    return;
}

template<bool t_y, bool t_hard>
auto div_flex_border_impl_t<t_y, t_hard>::get_pos() const -> pos_2D {
    return m_border.get_pos();
}

template<bool t_y, bool t_hard>
auto div_flex_border_impl_t<t_y, t_hard>::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ this->m_mutex };
    GUI_div_flex_impl_t<t_y, t_hard>::set_pos(pos + size_2D{ m_border.get_border_size().x, m_border.get_border_size().x }
    + size_2D{ GUI_div_flex_impl_t<t_y, t_hard>::get_margin().x, GUI_div_flex_impl_t<t_y, t_hard>::get_margin().x });
    m_border.set_pos(pos);
    return;
}

template<bool t_y, bool t_hard>
auto div_flex_border_impl_t<t_y, t_hard>::get_size() const -> size_2D {
    return m_border.get_size();
}

template<bool t_y, bool t_hard>
auto div_flex_border_impl_t<t_y, t_hard>::set_size(size_2D size) -> void {
    std::unique_lock lock{ this->m_mutex };
    GUI_div_impl_t<t_y, t_hard>::set_size(size - size_2D{ m_border.get_border_size().x * 2, m_border.get_border_size().x * 2 }
    - size_2D{ GUI_div_flex_impl_t<t_y, t_hard>::get_margin().x * 2, GUI_div_flex_impl_t<t_y, t_hard>::get_margin().x * 2 });
    m_border.set_size(size);
    return;
}

template<bool t_y, bool t_hard>
auto div_flex_border_impl_t<t_y, t_hard>::get_div_size() const -> size_2D {
    return GUI_div_impl_t<t_y, t_hard>::get_div_size();
}

template<bool t_y, bool t_hard>
auto div_flex_border_impl_t<t_y, t_hard>::set_div_size(size_2D size) -> void {
    std::unique_lock lock{ this->m_mutex };
    GUI_div_impl_t<t_y, t_hard>::set_size(size);
    m_border.set_size(size + size_2D{ m_border.get_border_size().x * 2, m_border.get_border_size().x * 2 }
    + size_2D{ GUI_div_flex_impl_t<t_y, t_hard>::get_margin().x * 2, GUI_div_flex_impl_t<t_y, t_hard>::get_margin().x * 2 });
    return;
}

template<bool t_y, bool t_hard>
auto div_flex_border_impl_t<t_y, t_hard>::get_margin() const -> size_1D {
    std::unique_lock lock{ this->m_mutex };
    return m_margin;
}

template<bool t_y, bool t_hard>
auto div_flex_border_impl_t<t_y, t_hard>::show_impl(bool base) -> void {
    if (!this->show_begin(base)) { return; }
    std::unique_lock lock{ this->m_mutex };
    GUI_div_flex_impl_t<t_y, t_hard>::show_impl(false);
    m_border.show();
    this->show_end(base);
    return;
}

template<bool t_y, bool t_hard>
auto div_flex_border_impl_t<t_y, t_hard>::hide_impl(bool base) -> void {
    if (!this->hide_begin(base)) { return; }
    std::unique_lock lock{ this->m_mutex };
    m_border.hide(false);
    GUI_div_flex_impl_t<t_y, t_hard>::hide_impl(false);
    this->hide_end(base);
    return;
}

//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways, color_t content_color, size_1D padding, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, color_t content_color, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
using divx_flex_soft_border_t = div_flex_border_impl_t<false, false>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways, color_t content_color, size_1D padding, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, color_t content_color, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
using divx_flex_hard_border_t = div_flex_border_impl_t<false, true>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways, color_t content_color, size_1D padding, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, color_t content_color, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
using divy_flex_soft_border_t = div_flex_border_impl_t<true, false>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways, color_t content_color, size_1D padding, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, color_t content_color, size_1D border_size, color_t border_color, size_1D margin, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
using divy_flex_hard_border_t = div_flex_border_impl_t<true, true>;

template<bool t_y>
class std_div_flex_border_impl_t: public div_flex_border_impl_t<t_y, true> {
public:
    static color_t content_color;
    static size_1D padding;
    static size_1D border_size;
    static size_1D margin;

    std_div_flex_border_impl_t() = default;
    std_div_flex_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways
    , color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large = false);
    std_div_flex_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin
    , color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large = false);
};

template<bool t_y>
color_t std_div_flex_border_impl_t<t_y>::content_color{ std_black };
template<bool t_y>
size_1D std_div_flex_border_impl_t<t_y>::padding{ 0 };
template<bool t_y>
size_1D std_div_flex_border_impl_t<t_y>::border_size{ std_line_size };
template<bool t_y>
size_1D std_div_flex_border_impl_t<t_y>::margin{ std_margin };

template<bool t_y>
std_div_flex_border_impl_t<t_y>::std_div_flex_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways
, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large)
: div_flex_border_impl_t<t_y, true>{ engine, depth_tracker, pos, size_sideways, content_color
, padding, border_size, border_color, margin, alignment, depth_range, from_large } {}

template<bool t_y>
std_div_flex_border_impl_t<t_y>::std_div_flex_border_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos
, div_create_from_member, size_1D object_size_sideways, size_1D object_margin
, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large)
: div_flex_border_impl_t<t_y, true>{ engine, depth_tracker, pos, div_create_from_member{}, object_size_sideways, object_margin
, content_color, border_size, border_color, margin, alignment, depth_range, from_large } {}

//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
using std_divx_flex_border_t = std_div_flex_border_impl_t<false>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, color_t border_color, alignment_2D alignment, depth_range_t depth_range, bool from_large = false
using std_divy_flex_border_t = std_div_flex_border_impl_t<true>;

#endif