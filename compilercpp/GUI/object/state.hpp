#ifndef COMPILERCPP_GUI_OBJECT_STATE
#define COMPILERCPP_GUI_OBJECT_STATE

#include <map>
#include <memory>
#include <cstdint>

#include "object_def.hpp"
#include "focus.hpp"
#include "depth_tracker.hpp"

#include "../../lib/stu.hpp"
#include "../primitive/engine_def.hpp"

class state_t {
private:
    struct instance_t {
        std::map<std::uint64_t, std::shared_ptr<GUI_object_t>> m_object;
        focus_t m_focus{};
    };

    engine_t* m_engine{};
    depth_tracker_t* m_depth_tracker{};
protected:
    instance_t m_current{};
    std::uint64_t m_current_id_object{};
private:
    std::uint64_t m_current_id_save{};
    std::map<std::uint64_t, instance_t> m_saved{};
public:
    state_t() = default;
    state_t(engine_t* engine, depth_tracker_t* depth_tracker);
    state_t(const state_t&) = delete;
    auto init(engine_t* engine, depth_tracker_t* depth_tracker) -> void;
    auto operator=(const state_t&) = delete;

    auto get_engine() -> engine_t*;
    auto get_depth_tracker() -> depth_tracker_t*;
    auto get_focus() -> focus_t*;

    template<typename t_object, typename... t_arg>
    auto add_object(t_arg&&... arg) -> std::uint64_t;
    template<typename t_object, typename t_self, typename... t_arg>
    auto add_object_weak(this t_self&& self, t_arg&&... arg) -> std::weak_ptr<t_object>;
    template<typename t_object, typename t_self, typename... t_arg>
    auto add_object_shared(this t_self&& self, t_arg&&... arg) -> std::shared_ptr<t_object>;
    template<typename t_object>
    auto get_object_weak(std::uint64_t id) -> std::weak_ptr<t_object>;
    template<typename t_object>
    auto get_object_shared(std::uint64_t id) -> std::shared_ptr<t_object>;
    auto remove_object(std::uint64_t id) -> void;
    auto save_state() -> std::uint64_t;
    auto clear_state() -> void;
    auto restore_state(std::uint64_t id) -> void;
    auto remove_state(std::uint64_t id) -> void;
};

state_t::state_t(engine_t* engine, depth_tracker_t* depth_tracker): m_engine{ engine }, m_depth_tracker{ depth_tracker } {}

auto state_t::init(engine_t* engine, depth_tracker_t* depth_tracker) -> void {
    m_engine = engine;
    m_depth_tracker = depth_tracker;
    return;
}

auto state_t::get_engine() -> engine_t* {
    return m_engine;
}

auto state_t::get_depth_tracker() -> depth_tracker_t* {
    return m_depth_tracker;
}

auto state_t::get_focus() -> focus_t* {
    return &m_current.m_focus;
}

template<typename t_object, typename... t_arg>
auto state_t::add_object(t_arg&&... arg) -> std::uint64_t {
    m_current.m_object.emplace(m_current_id_object, std::make_shared<t_object>(m_engine, m_depth_tracker, std::forward<t_arg>(arg)...));
    m_current.m_object[m_current_id_object]->show();
    return m_current_id_object++;
}

template<typename t_object, typename t_self, typename... t_arg>
auto state_t::add_object_weak(this t_self&& self, t_arg&&... arg) -> std::weak_ptr<t_object> {
    std::uint64_t id{ self.template add_object<t_object>(std::forward<t_arg>(arg)...) };
    return self.template get_object_weak<t_object>(id);
}

template<typename t_object, typename t_self, typename... t_arg>
auto state_t::add_object_shared(this t_self&& self, t_arg&&... arg) -> std::shared_ptr<t_object> {
    std::uint64_t id{ self.template add_object<t_object>(std::forward<t_arg>(arg)...) };
    return self.template get_object_shared<t_object>(id);
}

template<typename t_object>
auto state_t::get_object_weak(std::uint64_t id) -> std::weak_ptr<t_object> {
    return std::dynamic_pointer_cast<t_object>(m_current.m_object[id]);
}

template<typename t_object>
auto state_t::get_object_shared(std::uint64_t id) -> std::shared_ptr<t_object> {
    return std::dynamic_pointer_cast<t_object>(m_current.m_object[id]);
}

auto state_t::remove_object(std::uint64_t id) -> void {
    m_current.m_object[id]->hide();
    m_current.m_object.erase(id);
    return;
}

auto state_t::save_state() -> std::uint64_t {
    m_saved.emplace(m_current_id_save, m_current);
    return m_current_id_save++;
}

auto state_t::clear_state() -> void {
    m_current.m_focus.clear();
    for (auto i{ m_current.m_object.begin() }; i != m_current.m_object.end(); ++i) {
        i->second->hide();
    }
    m_current = {};
    return;
}

auto state_t::restore_state(std::uint64_t id) -> void {
    clear_state();
    m_current = m_saved[id];
    for (auto i{ m_current.m_object.begin() }; i != m_current.m_object.end(); ++i) {
        i->second->show();
    }
    return;
}

auto state_t::remove_state(std::uint64_t id) -> void {
    m_saved.erase(id);
    return;
}

#endif