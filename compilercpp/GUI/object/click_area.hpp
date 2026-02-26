#ifndef COMPILERCPP_GUI_OBJECT_CLICK_AREA
#define COMPILERCPP_GUI_OBJECT_CLICK_AREA

#include <functional>

#include "object_def.hpp"

#include "../../lib/stu.hpp"
#include "../../lib/common.hpp"
#include "../primitive/engine_def.hpp"

class click_area_t: public GUI_object_t {
private:
    engine_t* m_engine{};
    depth_tracker_t* m_depth_tracker{};
    std::uint64_t m_depth_id{};
    pos_2D m_pos{};
    size_2D m_size{};
    float m_depth{};
    std::function<void(pos_2D)> m_mouse_move_callback{};
    std::function<void(void)> m_mouse_leave_callback{};
    std::function<void(pos_2D)> m_mouse_left_click_callback{};
    std::function<void(pos_2D)> m_mouse_left_release_callback{};
    std::function<void(void)> m_callback{};
    std::function<void(pos_2D)>* m_mouse_move_callback_key{};
    std::function<void(void)>* m_mouse_leave_callback_key{};
    std::function<void(pos_2D)>* m_mouse_left_click_callback_key{};
    std::function<void(pos_2D)>* m_mouse_left_release_callback_key{};
    std::atomic_bool m_inside{ false };
    std::atomic_bool m_clicked_inside{ false };
    
    auto click_area_mouse_move_callback(pos_2D pos) -> void;
    auto click_area_mouse_leave_callback() -> void;
    auto click_area_mouse_left_click_callback(pos_2D pos) -> void;
    auto click_area_mouse_left_release_callback(pos_2D pos) -> void;
public:
    click_area_t() = default;
    click_area_t(const click_area_t& in);
    click_area_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, float depth
    , std::function<void(pos_2D)> mouse_move_callback, std::function<void(void)> mouse_leave_callback
    , std::function<void(pos_2D)> mouse_left_click_callback, std::function<void(pos_2D)> mouse_left_release_callback, std::function<void(void)> callback);
    auto operator=(const click_area_t& in) -> click_area_t&;

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto set_size(size_2D size) -> void override;
    auto get_margin() const -> size_1D override;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
};

click_area_t::click_area_t(const click_area_t& in): click_area_t{ in.m_engine, in.m_depth_tracker, in.m_pos, in.m_size, in.m_depth
, in.m_mouse_move_callback, in.m_mouse_leave_callback, in.m_mouse_left_click_callback, in.m_mouse_left_release_callback, in.m_callback} {}

click_area_t::click_area_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D size, float depth
, std::function<void(pos_2D)> mouse_move_callback, std::function<void(void)> mouse_leave_callback
, std::function<void(pos_2D)> mouse_left_click_callback, std::function<void(pos_2D)> mouse_left_release_callback, std::function<void(void)> callback)
: m_engine{ engine }, m_depth_tracker{ depth_tracker }, m_pos{ pos }, m_size{ size }, m_depth{ depth }
, m_mouse_move_callback{ mouse_move_callback }, m_mouse_leave_callback{ mouse_leave_callback }
, m_mouse_left_click_callback{ mouse_left_click_callback }, m_mouse_left_release_callback{ mouse_left_release_callback }, m_callback{ callback } {}

auto click_area_t::operator=(const click_area_t& in) -> click_area_t& {
    m_engine = in.m_engine;
    m_depth_tracker = in.m_depth_tracker;
    m_pos = in.m_pos;
    m_size = in.m_size;
    m_depth = in.m_depth;
    m_mouse_move_callback = in.m_mouse_move_callback;
    m_mouse_leave_callback = in.m_mouse_leave_callback;
    m_mouse_left_click_callback = in.m_mouse_left_click_callback;
    m_mouse_left_release_callback = in.m_mouse_left_release_callback;
    m_callback = in.m_callback;
    return *this;
}

auto click_area_t::click_area_mouse_move_callback(pos_2D pos) -> void {
    if (inside(m_pos, m_size, pos) && m_depth_tracker->top(pos, m_depth_id)) {
        m_inside = true;
        if (GetAsyncKeyState(VK_LBUTTON) >= 0) {
            std::unique_lock lock{ m_mutex };
            m_mouse_move_callback(pos);
        }
    }
    else {
        if (m_inside) {
            std::unique_lock lock{ m_mutex };
            m_mouse_leave_callback();
            m_inside = false;
        }
        if (GetAsyncKeyState(VK_LBUTTON) < 0) {
            m_clicked_inside = false;
        }
    }
    return;
}

auto click_area_t::click_area_mouse_leave_callback() -> void {
    if (m_inside) {
        std::unique_lock lock{ m_mutex };
        m_mouse_leave_callback();
        m_inside = false;
    }
    return;
}

auto click_area_t::click_area_mouse_left_click_callback(pos_2D pos) -> void {
    if (m_inside) {
        std::unique_lock lock{ m_mutex };
        m_mouse_left_click_callback(pos);
        m_clicked_inside = true;
    }
    return;
}

auto click_area_t::click_area_mouse_left_release_callback(pos_2D pos) -> void {
    if (m_inside) {
        std::unique_lock lock{ m_mutex };
        m_mouse_left_release_callback(pos);
        if (m_clicked_inside) {
            auto callback_copy{ m_callback };
            lock.unlock();
            callback_copy();
        }
    }
    return;
}

auto click_area_t::get_pos() const -> pos_2D {
    std::unique_lock lock{ m_mutex };
    return m_pos;
}

auto click_area_t::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ m_mutex };
    m_pos = pos;
    return;
}

auto click_area_t::get_size() const -> size_2D {
    std::unique_lock lock{ m_mutex };
    return m_size;
}

auto click_area_t::set_size(size_2D size) -> void {
    GUI_object_t::set_size(size);
    std::unique_lock lock{ m_mutex };
    m_size = size;
    return;
}

auto click_area_t::get_margin() const -> size_1D {
    return size_1D{ 0 };
}

auto click_area_t::show_impl(bool base) -> void {
    if (!show_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    m_depth_id = m_depth_tracker->add(m_pos, m_size, m_depth, true);
    m_mouse_move_callback_key = m_engine->add_mouse_move(std::bind(&click_area_t::click_area_mouse_move_callback, this, std::placeholders::_1));
    m_mouse_leave_callback_key = m_engine->add_mouse_leave(std::bind(&click_area_t::click_area_mouse_leave_callback, this));
    m_mouse_left_click_callback_key = m_engine->add_mouse_left_click(std::bind(&click_area_t::click_area_mouse_left_click_callback, this, std::placeholders::_1));
    m_mouse_left_release_callback_key = m_engine->add_mouse_left_release(std::bind(&click_area_t::click_area_mouse_left_release_callback, this, std::placeholders::_1));
    click_area_mouse_move_callback(m_engine->get_cursor_pos());
    show_end(base);
    return;
}

auto click_area_t::hide_impl(bool base) -> void {
    if (!hide_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    m_engine->remove_mouse_move(m_mouse_move_callback_key);
    m_engine->remove_mouse_leave(m_mouse_leave_callback_key);
    m_engine->remove_mouse_left_click(m_mouse_left_click_callback_key);
    m_engine->remove_mouse_left_release(m_mouse_left_release_callback_key);
    m_depth_tracker->remove(m_depth_id);
    hide_end(base);
    return;
}

#endif