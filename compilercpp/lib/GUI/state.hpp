#ifndef COMPILERCPP_LIB_GUI_STATE
#define COMPILERCPP_LIB_GUI_STATE

#include <map>
#include <memory>
#include <cstdint>

#include "object.hpp"

#include "../other/stu.hpp"
#include "../GUI_primitive/engine_def.hpp"

class state_t {
private:
    using instance_t = std::map<std::uint64_t, std::shared_ptr<GUI_object>>;

    engine_t* m_engine{};
    instance_t m_current{};
    std::uint64_t m_current_id_object{};
    std::uint64_t m_current_id_save{};
    std::map<std::uint64_t, instance_t> m_saved{};
public:
    state_t(engine_t* engine);
    state_t(const state_t&) = delete;
    ~state_t();
    auto operator=(const state_t&) = delete;

    auto get_engine() -> engine_t*;

    template<typename t_object, typename... t_arg>
    auto add_object(t_arg&&... arg) -> std::uint64_t;
    template<typename t_object>
    auto get_object(std::uint64_t id) -> std::weak_ptr<t_object>;
    auto remove_object(std::uint64_t id) -> void;
    auto save_state() -> std::uint64_t;
    auto clear_state() -> void;
    auto restore_state(std::uint64_t id) -> void;
    auto remove_state(std::uint64_t id) -> void;
};

state_t::state_t(engine_t* engine): m_engine{ engine } {}

state_t::~state_t() {
    clear_state();
    return;
}

auto state_t::get_engine() -> engine_t* {
    return m_engine;
}

template<typename t_object, typename... t_arg>
auto state_t::add_object(t_arg&&... arg) -> std::uint64_t {
    m_current.emplace(m_current_id_object, std::make_shared<t_object>(m_engine, std::forward<t_arg&&>(arg)...));
    m_current[m_current_id_object]->show();
    return m_current_id_object++;
}

template<typename t_object>
auto state_t::get_object(std::uint64_t id) -> std::weak_ptr<t_object> {
    return std::dynamic_pointer_cast<t_object>(m_current[id]);
}

auto state_t::remove_object(std::uint64_t id) -> void {
    m_current[id]->hide();
    m_current.erase(id);
    return;
}

auto state_t::save_state() -> std::uint64_t {
    m_saved.emplace(m_current_id_save, m_current);
    return m_current_id_save++;
}

auto state_t::clear_state() -> void {
    for (auto i{ m_current.begin() }; i != m_current.end(); ++i) {
        i->second->hide();
    }
    m_current = {};
    return;
}

auto state_t::restore_state(std::uint64_t id) -> void {
    clear_state();
    m_current = m_saved[id];
    for (auto i{ m_current.begin() }; i != m_current.end(); ++i) {
        i->second->show();
    }
    return;
}

auto state_t::remove_state(std::uint64_t id) -> void {
    m_saved.erase(id);
    return;
}

#endif