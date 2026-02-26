#ifndef COMPILERCPP_LIB_GUI_STATE
#define COMPILERCPP_LIB_GUI_STATE

#include <map>
#include <memory>
#include <cstdint>

#include "stu.hpp"
#include "engine_def.hpp"
#include "text_def.hpp"
#include "button_def.hpp"

class state_t {
private:
    struct instance_t {
        std::map<std::uint64_t, std::shared_ptr<text_t>> m_text{};
        std::map<std::uint64_t, std::shared_ptr<button_t>> m_button{};
    };

    engine_t* m_engine{};
    instance_t m_current{};
    std::uint64_t m_current_id{};
    std::map<std::uint64_t, instance_t> m_saved{};
public:
    state_t(engine_t* engine);
    state_t(const state_t&) = delete;
    ~state_t();
    auto operator=(const state_t&) = delete;

    auto add_text(std::wstring m_text, pos_2D pos, size_2D size, size_1D size_font, color_t color, alignment_2D alignment) -> std::uint64_t;
    auto remove_text(std::uint64_t id) -> void;
    auto add_button(pos_2D pos, size_2D size, size_1D border_size
    , std::wstring display_text, depth_range_t depth_range, std::function<void(void)> callback) -> std::uint64_t;
    auto remove_button(std::uint64_t id) -> void;
    auto save_state(std::uint64_t id) -> void;
    auto clear_state() -> void;
    auto restore_state(std::uint64_t id) -> void;
    auto remove_state(std::uint64_t id) -> void;
};

state_t::state_t(engine_t* engine): m_engine{ engine } {}

state_t::~state_t() {
    clear_state();
    return;
}

auto state_t::add_text(std::wstring m_text, pos_2D pos, size_2D size, size_1D size_font, color_t color, alignment_2D alignment) -> std::uint64_t {
    m_current.m_text.emplace(m_current_id, std::make_shared<text_t>(m_engine, m_text, pos, size, size_font, color, alignment));
    m_current.m_text[m_current_id]->show();
    return m_current_id++;
}

auto state_t::remove_text(std::uint64_t id) -> void {
    m_current.m_text[id]->hide();
    m_current.m_text.erase(id);
    return;
}

auto state_t::add_button(pos_2D pos, size_2D size, size_1D border_size
, std::wstring display_text, depth_range_t depth_range, std::function<void(void)> callback) -> std::uint64_t {
    m_current.m_button.emplace(m_current_id, std::make_shared<button_t>(m_engine, pos, size, border_size, display_text, depth_range, callback));
    m_current.m_button[m_current_id]->show();
    return m_current_id++;
}

auto state_t::remove_button(std::uint64_t id) -> void {
    m_current.m_button[id]->hide();
    m_current.m_button.erase(id);
    return;
}

auto state_t::save_state(std::uint64_t id) -> void {
    m_saved.emplace(id, m_current);
    return;
}

auto state_t::clear_state() -> void {
    for (auto i{ m_current.m_button.begin() }; i != m_current.m_button.end(); ++i) {
        i->second->hide();
    }
    for (auto i{ m_current.m_text.begin() }; i != m_current.m_text.end(); ++i) {
        i->second->hide();
    }
    m_current = {};
    return;
}

auto state_t::restore_state(std::uint64_t id) -> void {
    m_current = m_saved[id];
    for (auto i{ m_current.m_button.begin() }; i != m_current.m_button.end(); ++i) {
        i->second->show();
    }
    return;
}

auto state_t::remove_state(std::uint64_t id) -> void {
    m_saved.erase(id);
    return;
}

#endif