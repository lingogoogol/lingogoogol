module lgo.GUI.object.state;

import std;

import lgo.io.file;
import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.focus;
import lgo.GUI.object.depth_tracker;

namespace lgo
{
    state_t::state_t(engine_t* engine, depth_tracker_t* depth_tracker)
    :
        m_engine{ engine },
        m_depth_tracker{ depth_tracker }
    {}

    auto state_t::init(engine_t* engine, depth_tracker_t* depth_tracker) -> void
    {
        m_engine = engine;
        m_depth_tracker = depth_tracker;
        return;
    }

    auto state_t::get_engine() -> engine_t*
    {
        return m_engine;
    }

    auto state_t::get_depth_tracker() -> depth_tracker_t*
    {
        return m_depth_tracker;
    }

    auto state_t::get_focus() -> focus_t*
    {
        return &m_current.m_focus;
    }

    auto state_t::remove_object(std::uint64_t id) -> void
    {
        m_current.m_object[id]->hide();
        m_current.m_object.erase(id);
        return;
    }

    auto state_t::save_state() -> std::uint64_t
    {
        m_saved.emplace(m_current_id_save, m_current);
        return m_current_id_save++;
    }

    auto state_t::clear_state() -> void
    {
        m_current.m_focus.clear();
        for (auto i{ m_current.m_object.begin() }; i != m_current.m_object.end(); ++i)
        {
            i->second->hide();
        }
        m_current = {};
        return;
    }

    auto state_t::restore_state(std::uint64_t id) -> void
    {
        clear_state();
        m_current = m_saved[id];
        for (auto i{ m_current.m_object.begin() }; i != m_current.m_object.end(); ++i)
        {
            i->second->show();
        }
        return;
    }

    auto state_t::remove_state(std::uint64_t id) -> void
    {
        m_saved.erase(id);
        return;
    }

    auto state_t::log(std::uint32_t tab_count) const -> void
    {
        log_file(tab(tab_count) + "{\n");
        log_file(tab(tab_count + 1) + "type: state_t\n");
        log_file(tab(tab_count + 1) + "current:\n");
        log_file(tab(tab_count + 1) + "[\n");
        for (auto i{ m_current.m_object.begin() }; i != m_current.m_object.end(); ++i)
        {
            log_file(tab(tab_count + 2) + "{\n");
            log_file(tab(tab_count + 3) + "id: " + std::to_string(i->first) + "\n");
            log_file(tab(tab_count + 3) + "object:\n");
            i->second->log(tab_count + 3);
            log_file(tab(tab_count + 2) + "}\n");
        }
        log_file(tab(tab_count + 1) + "]\n");
        log_file(tab(tab_count) + "}\n");
        return;
    }
}
