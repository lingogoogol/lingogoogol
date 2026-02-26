#ifndef COMPILERCPP_GUI_OBJECT_DIV_DEF
#define COMPILERCPP_GUI_OBJECT_DIV_DEF

#include <map>

#include "div_decl.hpp"
#include "object_def.hpp"

#include "../primitive/engine_def.hpp"
#include "../../lib/stu.hpp"

class GUI_div_t: public GUI_object_t, public state_t {
protected:
    GUI_div_t() = default;
    GUI_div_t(engine_t* engine, depth_tracker_t* depth_tracker);
public:
    virtual auto child_set_size(std::uint64_t id, size_2D size) -> void = 0;
};

GUI_div_t::GUI_div_t(engine_t* engine, depth_tracker_t* depth_tracker): state_t{ engine, depth_tracker } {}

template<bool t_y, bool t_hard>
class GUI_div_impl_t: public GUI_div_t {
private:
    pos_2D m_pos{};
    size_2D m_size{};
    size_1D m_margin{};
    std::uint8_t m_alignment_forward{}, m_alignment_sideways{};
    bool m_from_large{ false };
    std::uint64_t m_length{ 0 };
    size_1D m_last_margin{ 0 };
    std::map<std::uint64_t, bool> m_ghost{};

    auto move_child(std::uint64_t begin, std::uint64_t end, size_2D offset) -> void;
protected:
    template<typename t_object, bool t_ghost, typename... t_arg>
    auto add_object_impl(t_arg&&... arg) -> std::uint64_t;
    auto child_set_size(std::uint64_t id, size_2D size) -> void override;
public:
    constexpr static bool hard = t_hard;

    GUI_div_impl_t() = default;
    GUI_div_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, size_1D margin, alignment_2D alignment, bool from_large = false);
    GUI_div_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways
    , size_1D object_margin, size_1D size_forward, alignment_2D alignment, bool from_large = false);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    virtual auto set_size(size_2D size) -> void;
    virtual auto get_div_size() const -> size_2D;
    virtual auto set_div_size(size_2D size) -> void;
    auto get_margin() const -> size_1D override;
    auto len() const -> size_1D;
    auto space_2D(const size_1D& margin) const -> size_2D;
    auto space_2D(const GUI_object_t& object) const -> size_2D;
    auto space_x(const size_1D& margin) const -> size_1D;
    auto space_x(const GUI_object_t& object) const -> size_1D;
    auto space_y(const size_1D& margin) const -> size_1D;
    auto space_y(const GUI_object_t& object) const -> size_1D;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;

    template<typename t_object, typename t_self, typename... t_arg>
    auto add_object(this t_self&& self, t_arg&&... arg) -> std::uint64_t;
    template<typename t_object, typename t_self, typename... t_arg>
    auto add_object_ghost(this t_self&& self, t_arg&&... arg) -> std::uint64_t;
    template<typename t_object, typename t_self, typename... t_arg>
    auto add_object_ghost_weak(this t_self&& self, t_arg&&... arg) -> std::weak_ptr<t_object>;
    template<typename t_object, typename t_self, typename... t_arg>
    auto add_object_ghost_shared(this t_self&& self, t_arg&&... arg) -> std::shared_ptr<t_object>;
    auto remove_object(std::uint64_t id) -> void;
    auto clear_state() -> void;
};

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::child_set_size(std::uint64_t id, size_2D size) -> void {
    std::unique_lock lock{ m_mutex };
    std::int64_t size_forward{ t_y ? size.y : size.x }, size_sideways{ t_y ? size.x : size.y };
    pos_2D pos_orig{ m_current.m_object[id]->get_pos() };
    size_2D size_orig{ m_current.m_object[id]->get_size() };
    std::int64_t size_orig_forward{ t_y ? size_orig.y : size_orig.x }, size_orig_sideways{ t_y ? size_orig.x : size_orig.y };
    std::int64_t offset_sideways{};
    std::int64_t div_size_sideways{ t_y ? get_size().x : get_size().y };
    switch (m_alignment_sideways) {
    case 0:
        offset_sideways = 0;
        break;
    case 1:
        offset_sideways = (div_size_sideways - size_sideways) / 2 - (div_size_sideways - size_orig_sideways) / 2;
        break;
    case 2:
        offset_sideways = size_orig_sideways - size_sideways;
        break;
    }
    m_current.m_object[id]->set_pos(pos_2D{ t_y ? pos_orig.x + offset_sideways : pos_orig.x, t_y ? pos_orig.y : pos_orig.y + offset_sideways });
    if (m_ghost[id]) {
        return;
    }
    switch (m_alignment_forward) {
    case 0: {
        m_length += size_forward - size_orig_forward;
        std::int64_t offset_forward{ size_forward - size_orig_forward };
        if (m_current.m_object.size()) {
            if (m_from_large) {
                move_child(0, id, size_2D{ t_y ? 0 : offset_forward, t_y ? offset_forward : 0 });
            }
            else {
                move_child(id + 1, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : offset_forward, t_y ? offset_forward : 0 });
            }
        }
        break;
    }
    case 1: {
        std::uint64_t length_orig{ m_length };
        m_length += size_forward - size_orig_forward;
        if (m_current.m_object.size()) {
            if (m_from_large) {
                std::int64_t offset_forward_large{ (size_forward + static_cast<std::int64_t>(m_length)) / 2 - (size_forward + static_cast<std::int64_t>(length_orig)) / 2 };
                std::int64_t offset_forward_small{ offset_forward_large - size_forward + size_orig_forward };
                move_child(0, id, size_2D{ t_y ? 0 : offset_forward_large, t_y ? offset_forward_large : 0 });
                move_child(id, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : offset_forward_small, t_y ? offset_forward_small : 0 });
            }
            else {
                std::int64_t offset_forward_small{ (size_forward - static_cast<std::int64_t>(m_length)) / 2 - (size_forward - static_cast<std::int64_t>(length_orig)) / 2 };
                std::int64_t offset_forward_large{ offset_forward_small + size_forward - size_orig_forward };
                move_child(0, id + 1, size_2D{ t_y ? 0 : offset_forward_small, t_y ? offset_forward_small : 0 });
                move_child(id + 1, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : offset_forward_large, t_y ? offset_forward_large : 0 });
            }
        }
        break;
    }
    case 2: {
        m_length += size_forward - size_orig_forward;
        std::int64_t offset_forward{ size_orig_forward - size_forward };
        if (m_current.m_object.size()) {
            if (m_from_large) {
                move_child(id, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : offset_forward, t_y ? offset_forward : 0 });
            }
            else {
                move_child(0, id + 1, size_2D{ t_y ? 0 : offset_forward, t_y ? offset_forward : 0 });
            }
        }
        break;
    }
    }
    return;
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::move_child(std::uint64_t begin, std::uint64_t end, size_2D offset) -> void {
    std::unique_lock lock{ m_mutex };
    auto end_it{ m_current.m_object.lower_bound(end) };
    for (auto i{ m_current.m_object.lower_bound(begin) }; i != end_it; ++i) {
        pos_2D pos_original{ i->second->get_pos() };
        i->second->set_pos(pos_original + offset);
    }
    return;
}

template<bool t_y, bool t_hard>
template<typename t_object, bool t_ghost, typename... t_arg>
auto GUI_div_impl_t<t_y, t_hard>::add_object_impl(t_arg&&... arg) -> std::uint64_t {
    std::unique_lock lock{ m_mutex };
    GUI_object_t* object{ new t_object{ get_engine(), get_depth_tracker(), pos_2D{ 0, 0 }, std::forward<t_arg&&>(arg)... } };
    std::int64_t object_pos_forward{}, object_pos_sideways{};
    std::int64_t margin_missed{ std::max(object->get_margin().x - m_last_margin.x, static_cast<std::int64_t>(0)) };
    std::int64_t margin_actual{ m_current.m_object.size() ? std::max(m_last_margin.x, object->get_margin().x) : 0 };
    std::int64_t pos_forward{ t_y ? m_pos.y : m_pos.x }, pos_sideways{ t_y ? m_pos.x : m_pos.y };
    std::int64_t object_size_forward{ t_y ? object->get_size().y : object->get_size().x }, object_size_sideways{ t_y ? object->get_size().x : object->get_size().y };
    std::int64_t size_forward{ t_y ? m_size.y : m_size.x }, size_sideways{ t_y ? m_size.x : m_size.y };
    switch (m_alignment_forward) {
    case 0: {
        if constexpr (!t_ghost) {
            m_length += (t_hard ? margin_missed : margin_actual);
        }
        if (m_from_large) {
            if constexpr (!t_ghost) {
                if (m_current.m_object.size()) {
                    move_child(0, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : object->get_size().x + (t_hard ? object->get_margin().x
                    + margin_missed : margin_actual), t_y ? object->get_size().y + (t_hard ? object->get_margin().x + margin_missed : margin_actual) : 0 });
                }
            }
            object_pos_forward = pos_forward + (t_ghost ? -object_size_forward - (t_hard ? margin_missed : margin_actual): t_hard ? object->get_margin().x : 0);
        }
        else {
            object_pos_forward = pos_forward + m_length + (t_ghost ? t_hard ? margin_missed : margin_actual : 0);
        }
        if constexpr (!t_ghost) {
            m_length += object_size_forward + (t_hard ? object->get_margin().x : 0);
        }
        break;
    }
    case 1: {
        std::uint64_t length_orig{ m_length };
        if constexpr (!t_ghost) {
            m_length += (t_hard ? margin_missed : margin_actual) + object_size_forward + (t_hard ? object->get_margin().x : 0);
        }
        if (m_from_large) {
            if constexpr (t_ghost) {
                object_pos_forward = pos_forward - (size_forward + static_cast<std::int64_t>(m_length)) / 2
                - (t_hard ? margin_missed : margin_actual) - object_size_forward;
            }
            else {
                if (m_current.m_object.size()) {
                    size_1D offset{ (size_forward + static_cast<std::int64_t>(m_length)) / 2 - (size_forward + static_cast<std::int64_t>(length_orig)) / 2 };
                    move_child(0, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : offset.x, t_y ? offset.x : 0 });
                }
                object_pos_forward = pos_forward + (size_forward + static_cast<std::int64_t>(m_length)) / 2
                - length_orig - (t_hard ? margin_missed : margin_actual) - object_size_forward;
            }
        }
        else {
            if constexpr (t_ghost) {
                object_pos_forward = pos_forward + (size_forward
                + static_cast<std::int64_t>(m_length)) / 2 + (t_hard ? margin_missed : margin_actual);
            }
            else {
                if (m_current.m_object.size()) {
                    size_1D offset{ (size_forward - static_cast<std::int64_t>(m_length)) / 2 - (size_forward - static_cast<std::int64_t>(length_orig)) / 2 };
                    move_child(0, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : offset.x, t_y ? offset.x : 0 });
                }
                object_pos_forward = pos_forward + (size_forward - static_cast<std::int64_t>(m_length)) / 2
                + length_orig + (t_hard ? margin_missed : margin_actual);
            }
        }
        break;
    }
    case 2: {
        if constexpr (!t_ghost) {
            m_length += (t_hard ? margin_missed : margin_actual) + object_size_forward;
        }
        if (m_from_large) {
            object_pos_forward = pos_forward + size_forward - m_length - (t_ghost ? (t_hard ? margin_missed : margin_actual) + object_size_forward : 0);
        }
        else {
            if constexpr (!t_ghost) {
                if (m_current.m_object.size()) {
                    move_child(0, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : -object->get_size().x - (t_hard ? margin_missed
                    + object->get_margin().x : margin_actual), t_y ? -object->get_size().y - (t_hard ? margin_missed + object->get_margin().x : margin_actual) : 0 });
                }
            }
            object_pos_forward = pos_forward + size_forward - object_size_forward - (t_hard ? object->get_margin().x : 0);
        }
        if constexpr (!t_ghost) {
            m_length += (t_hard ? object->get_margin().x : 0);
        }
        break;
    }
    }
    switch (m_alignment_sideways) {
    case 0:
        object_pos_sideways = pos_sideways + (t_hard ? object->get_margin().x : 0);
        break;
    case 1:
        object_pos_sideways = pos_sideways + (size_sideways - object_size_sideways) / 2;
        break;
    case 2:
        object_pos_sideways = pos_sideways + size_sideways - object_size_sideways - (t_hard ? object->get_margin().x : 0);
        break;
    }
    m_current.m_object.emplace(m_current_id_object, object);
    object->set_pos(pos_2D{ t_y ? object_pos_sideways : object_pos_forward, t_y ? object_pos_forward : object_pos_sideways });
    object->set_size_callback(std::bind(&GUI_div_impl_t<t_y, t_hard>::child_set_size, this, m_current_id_object, std::placeholders::_1));
    if (status() == showed || status() == showing) {
        object->show();
    }
    if constexpr (!t_ghost) {
        m_last_margin = object->get_margin();
    }
    m_ghost.emplace(m_current_id_object, t_ghost);
    return m_current_id_object++;
}

template<bool t_y, bool t_hard>
GUI_div_impl_t<t_y, t_hard>::GUI_div_impl_t(engine_t* engine, depth_tracker_t* depth_tracker
, pos_2D pos, size_2D size, size_1D margin, alignment_2D alignment, bool from_large)
: GUI_div_t{ engine, depth_tracker }, m_pos{ pos }, m_size{ size }, m_margin{ margin }
, m_alignment_forward{ t_y ? static_cast<std::uint8_t>(alignment.y) : static_cast<std::uint8_t>(alignment.x) }
, m_alignment_sideways{ t_y ? static_cast<std::uint8_t>(alignment.x) : static_cast<std::uint8_t>(alignment.y) }, m_from_large{ from_large } {}

template<bool t_y, bool t_hard>
GUI_div_impl_t<t_y, t_hard>::GUI_div_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways
, size_1D object_margin, size_1D size_forward, alignment_2D alignment, bool from_large)
: GUI_div_impl_t{ engine, depth_tracker, pos, size_2D{ t_y ? object_size_sideways.x + (t_hard ? object_margin.x * 2 : 0) : size_forward.x
, t_y ? size_forward.x : object_size_sideways.x + (t_hard ? object_margin.x * 2 : 0) }, t_hard ? size_1D{ 0 } : object_margin, alignment, from_large } {}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::get_pos() const -> pos_2D {
    std::unique_lock lock{ m_mutex };
    return m_pos;
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ m_mutex };
    size_2D offset{ pos - m_pos };
    m_pos = pos;
    for (auto i{ m_current.m_object.begin() }; i != m_current.m_object.end(); ++i) {
        i->second->set_pos(i->second->get_pos() + offset);
    }
    return;
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::get_size() const -> size_2D {
    return GUI_div_impl_t<t_y, t_hard>::get_div_size();
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::set_size(size_2D size) -> void {
    GUI_div_impl_t<t_y, t_hard>::set_div_size(size);
    return;
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::get_div_size() const -> size_2D {
    std::unique_lock lock{ m_mutex };
    return m_size;
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::set_div_size(size_2D size) -> void {
    std::unique_lock lock{ m_mutex };
    GUI_div_t::set_size(size);
    std::int64_t size_forward{ t_y ? size.y : size.x }, size_sideways{ t_y ? size.x : size.y };
    std::int64_t size_orig_forward{ t_y ? m_size.y : m_size.x };
    m_size = size;
    switch (m_alignment_forward) {
    case 1: {
        size_1D offset{ m_from_large ? (size_forward + static_cast<std::int64_t>(m_length)) / 2 - (size_orig_forward + static_cast<std::int64_t>(m_length)) / 2
        : (size_forward - static_cast<std::int64_t>(m_length)) / 2 - (size_orig_forward - static_cast<std::int64_t>(m_length)) / 2 };
        move_child(0, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : offset.x, t_y ? offset.x : 0 });
        break;
    }
    case 2: {
        size_1D offset{ size_forward - size_orig_forward };
        move_child(0, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : offset.x, t_y ? offset.x : 0 });
        break;
    }
    }
    switch (m_alignment_sideways) {
    case 1:
        for (auto i{ m_current.m_object.begin() }; i != m_current.m_object.end(); ++i) {
            std::int64_t object_size_sideways{ t_y ? i->second->get_pos().x : i->second->get_pos().y };
            size_1D offset{ (size_sideways - object_size_sideways) / 2 - (size_orig_forward - object_size_sideways) / 2 };
            i->second->set_pos(pos_2D{ t_y ? offset.x : 0, t_y ? 0 : offset.x });
        }
        break;
    case 2:
        size_1D offset{ size_sideways - size_orig_forward };
        move_child(0, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? offset.x : 0, t_y ? 0 : offset.x });
        break;
    }
    return;
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::get_margin() const -> size_1D {
    std::unique_lock lock{ m_mutex };
    return m_margin;
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::len() const -> size_1D {
    std::unique_lock lock{ m_mutex };
    return size_1D{ static_cast<std::int64_t>(m_length) };
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::space_2D(const size_1D& margin) const -> size_2D {
    std::unique_lock lock{ m_mutex };
    return size_2D{ space_x(margin).x, space_y(margin).x };
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::space_2D(const GUI_object_t& object) const -> size_2D {
    std::unique_lock lock{ m_mutex };
    return size_2D{ space_x(object).x, space_y(object).x };
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::space_x(const size_1D& margin) const -> size_1D {
    std::unique_lock lock{ m_mutex };
    std::int64_t margin_missed{ std::max(margin.x - m_last_margin.x, static_cast<std::int64_t>(0)) };
    std::int64_t margin_actual{ m_current.m_object.size() ? std::max(m_last_margin.x, margin.x) : 0 };
    return size_1D{ static_cast<std::int64_t>(t_y ? m_size.x - (hard ? margin.x * 2 : 0) : m_size.x - m_length - (t_hard ? margin_missed + margin.x : margin_actual)) };
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::space_x(const GUI_object_t& object) const -> size_1D {
    return space_x(object.get_margin());
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::space_y(const size_1D& margin) const -> size_1D {
    std::unique_lock lock{ m_mutex };
    std::int64_t margin_missed{ std::max(margin.x - m_last_margin.x, static_cast<std::int64_t>(0)) };
    std::int64_t margin_actual{ m_current.m_object.size() ? std::max(m_last_margin.x, margin.x) : 0 };
    return size_1D{ static_cast<std::int64_t>(t_y ? m_size.y - m_length - (t_hard ? margin_missed + margin.x : margin_actual) : m_size.y - (hard ? margin.x * 2 : 0)) };
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::space_y(const GUI_object_t& object) const -> size_1D {
    return space_y(object.get_margin());
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::show_impl(bool base) -> void {
    if (!show_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    for (auto i{ m_current.m_object.begin() }; i != m_current.m_object.end(); ++i) {
        i->second->show();
    }
    show_end(base);
    return;
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::hide_impl(bool base) -> void {
    if (!hide_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    for (auto i{ m_current.m_object.begin() }; i != m_current.m_object.end(); ++i) {
        i->second->hide();
    }
    hide_end(base);
    return;
}

template<bool t_y, bool t_hard>
template<typename t_object, typename t_self, typename... t_arg>
auto GUI_div_impl_t<t_y, t_hard>::add_object(this t_self&& self, t_arg&&... arg) -> std::uint64_t {
    return self.add_object_impl<t_object, false>(std::forward<t_arg&&>(arg)...);
}

template<bool t_y, bool t_hard>
template<typename t_object, typename t_self, typename... t_arg>
auto GUI_div_impl_t<t_y, t_hard>::add_object_ghost(this t_self&& self, t_arg&&... arg) -> std::uint64_t {
    return self.add_object_impl<t_object, true>(std::forward<t_arg&&>(arg)...);
}

template<bool t_y, bool t_hard>
template<typename t_object, typename t_self, typename... t_arg>
auto GUI_div_impl_t<t_y, t_hard>::add_object_ghost_weak(this t_self&& self, t_arg&&... arg) -> std::weak_ptr<t_object> {
    std::unique_lock lock{ self.m_mutex };
    std::uint64_t id{ self.add_object_ghost<t_object>(std::forward<t_arg&&>(arg)...) };
    return self.get_object_weak<t_object>(id);
}

template<bool t_y, bool t_hard>
template<typename t_object, typename t_self, typename... t_arg>
auto GUI_div_impl_t<t_y, t_hard>::add_object_ghost_shared(this t_self&& self, t_arg&&... arg) -> std::shared_ptr<t_object> {
    std::unique_lock lock{ self.m_mutex };
    std::uint64_t id{ self.add_object_ghost<t_object>(std::forward<t_arg&&>(arg)...) };
    return self.get_object_shared<t_object>(id);
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::remove_object(std::uint64_t id) -> void {
    std::unique_lock lock{ m_mutex };
    std::int64_t size_forward{ t_y ? m_size.y : m_size.x };
    size_2D size_orig{ m_current.m_object[id]->get_size() };
    auto current_iter{ m_current.m_object.find(id) };
    auto next_iter{ std::next(current_iter) };
    std::int64_t last_margin_orig{}, next_margin_orig{}, last_margin{}, next_margin{};
    if (current_iter == m_current.m_object.begin()) {
        last_margin = 0;
        last_margin_orig = (t_hard ? current_iter->second->get_margin().x : 0);
    }
    else {
        last_margin = ((next_iter == m_current.m_object.end() && !t_hard)
        ? 0 : std::prev(current_iter)->second->get_margin().x);
        last_margin_orig = std::max(last_margin, current_iter->second->get_margin().x);
    }
    if (next_iter == m_current.m_object.end()) {
        next_margin = 0;
        next_margin_orig = (t_hard ? current_iter->second->get_margin().x : 0);
    }
    else {
        next_margin = ((current_iter == m_current.m_object.begin() && !t_hard)
        ? 0 : next_iter->second->get_margin().x);
        next_margin_orig = std::max(next_margin, current_iter->second->get_margin().x);
    }
    std::int64_t margin_actual{ std::max(last_margin, next_margin) - std::max(last_margin_orig, next_margin_orig) };
    std::int64_t size_orig_forward{ t_y ? size_orig.y : size_orig.x };
    state_t::remove_object(id);
    if (m_ghost[id]) {
        return;
    }
    switch (m_alignment_forward) {
    case 0: {
        std::int64_t offset_forward{ margin_actual - size_orig_forward };
        m_length += offset_forward;
        if (m_current.m_object.size()) {
            if (m_from_large) {
                move_child(0, id, size_2D{ t_y ? 0 : offset_forward, t_y ? offset_forward : 0 });
            }
            else {
                move_child(id + 1, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : offset_forward, t_y ? offset_forward : 0 });
            }
        }
        break;
    }
    case 1: {
        std::uint64_t length_orig{ m_length };
        m_length += margin_actual - size_orig_forward;
        if (m_current.m_object.size()) {
            if (m_from_large) {
                std::int64_t offset_forward_large{ (size_forward + static_cast<std::int64_t>(m_length)) / 2 - (size_forward + static_cast<std::int64_t>(length_orig)) / 2 };
                std::int64_t offset_forward_small{ offset_forward_large - size_forward + size_orig_forward };
                move_child(0, id, size_2D{ t_y ? 0 : offset_forward_large, t_y ? offset_forward_large : 0 });
                move_child(id, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : offset_forward_small, t_y ? offset_forward_small : 0 });
            }
            else {
                std::int64_t offset_forward_small{ (size_forward - static_cast<std::int64_t>(m_length)) / 2 - (size_forward - static_cast<std::int64_t>(length_orig)) / 2 };
                std::int64_t offset_forward_large{ offset_forward_small + size_forward - size_orig_forward };
                move_child(0, id + 1, size_2D{ t_y ? 0 : offset_forward_small, t_y ? offset_forward_small : 0 });
                move_child(id + 1, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : offset_forward_large, t_y ? offset_forward_large : 0 });
            }
        }
        break;
    }
    case 2: {
        std::int64_t offset_forward{ margin_actual - size_orig_forward };
        m_length += margin_actual - size_orig_forward;
        if (m_current.m_object.size()) {
            if (m_from_large) {
                move_child(id, m_current.m_object.rbegin()->first + 1, size_2D{ t_y ? 0 : offset_forward, t_y ? offset_forward : 0 });
            }
            else {
                move_child(0, id + 1, size_2D{ t_y ? 0 : offset_forward, t_y ? offset_forward : 0 });
            }
        }
        break;
    }
    }
    return;
}

template<bool t_y, bool t_hard>
auto GUI_div_impl_t<t_y, t_hard>::clear_state() -> void {
    std::unique_lock lock{ m_mutex };
    state_t::clear_state();
    m_length = 0;
    return;
}

//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, size_1D margin, alignment_2D alignment, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, size_1D size_forward, alignment_2D alignment, bool from_large = false
using divx_soft_t = GUI_div_impl_t<false, false>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, size_1D margin, alignment_2D alignment, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, size_1D size_forward, alignment_2D alignment, bool from_large = false
using divx_hard_t = GUI_div_impl_t<false, true>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, size_1D margin, alignment_2D alignment, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, size_1D size_forward, alignment_2D alignment, bool from_large = false
using divy_soft_t = GUI_div_impl_t<true, false>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, size_1D margin, alignment_2D alignment, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, size_1D size_forward, alignment_2D alignment, bool from_large = false
using divy_hard_t = GUI_div_impl_t<true, true>;

template<bool t_y, bool t_hard>
class GUI_div_flex_impl_t: public GUI_div_impl_t<t_y, t_hard> {
protected:
    template<typename t_object, bool t_ghost, typename... t_arg>
    auto add_object_impl(t_arg&&... arg) -> std::uint64_t;
    auto child_set_size(std::uint64_t id, size_2D size) -> void override;

    template<bool t_y, bool t_hard>
    friend class GUI_div_impl_t;
public:
    GUI_div_flex_impl_t() = default;
    GUI_div_flex_impl_t(engine_t* engine, depth_tracker_t* depth_tracker
    , pos_2D pos, size_1D size_sideways, size_1D margin, alignment_2D alignment, bool from_large = false);
    GUI_div_flex_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member
    , size_1D object_size_sideways, size_1D object_margin, alignment_2D alignment, bool from_large = false);
};

template<bool t_y, bool t_hard>
template<typename t_object, bool t_ghost, typename... t_arg>
auto GUI_div_flex_impl_t<t_y, t_hard>::add_object_impl(t_arg&&... arg) -> std::uint64_t {
    std::unique_lock lock{ this->m_mutex };
    std::uint64_t out{ GUI_div_impl_t<t_y, t_hard>::template add_object_impl<t_object, t_ghost>(std::forward<t_arg&&>(arg)...) };
    this->set_div_size(size_2D{ t_y ? this->get_div_size().x : this->len().x, t_y ? this->len().x : this->get_div_size().y });
    return out;
}

template<bool t_y, bool t_hard>
auto GUI_div_flex_impl_t<t_y, t_hard>::child_set_size(std::uint64_t id, size_2D size) -> void {
    std::unique_lock lock{ this->m_mutex };
    GUI_div_impl_t<t_y, t_hard>::child_set_size(id, size);
    this->set_div_size(size_2D{ t_y ? this->get_div_size().x : this->len().x, t_y ? this->len().x : this->get_div_size().y });
    return;
}

template<bool t_y, bool t_hard>
GUI_div_flex_impl_t<t_y, t_hard>::GUI_div_flex_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos
, size_1D size_sideways, size_1D margin, alignment_2D alignment, bool from_large)
: GUI_div_impl_t<t_y, t_hard>{ engine, depth_tracker, pos, size_2D{ t_y ? size_sideways.x : 0, t_y ? 0 : size_sideways.x }, margin, alignment, from_large } {}

template<bool t_y, bool t_hard>
GUI_div_flex_impl_t<t_y, t_hard>::GUI_div_flex_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member
, size_1D object_size_sideways, size_1D object_margin, alignment_2D alignment, bool from_large)
: GUI_div_impl_t<t_y, t_hard>{ engine, depth_tracker, pos, div_create_from_member{}, object_size_sideways, object_margin, size_1D{ 0 }, alignment, from_large } {}

//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways, size_1D margin, alignment_2D alignment, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, alignment_2D alignment, bool from_large = false
using divx_flex_soft_t = GUI_div_flex_impl_t<false, false>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways, size_1D margin, alignment_2D alignment, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, alignment_2D alignment, bool from_large = false
using divx_flex_hard_t = GUI_div_flex_impl_t<false, true>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways, size_1D margin, alignment_2D alignment, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, alignment_2D alignment, bool from_large = false
using divy_flex_soft_t = GUI_div_flex_impl_t<true, false>;
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_1D size_sideways, size_1D margin, alignment_2D alignment, bool from_large = false
//
//engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, div_create_from_member, size_1D object_size_sideways, size_1D object_margin, alignment_2D alignment, bool from_large = false
using divy_flex_hard_t = GUI_div_flex_impl_t<true, true>;

#endif