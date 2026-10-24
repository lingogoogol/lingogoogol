module lgo.GUI.object.click_area;

import std;

import external.GLFW;

import lgo.io.file;
import lgo.math.vec;
import lgo.math.op;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.depth_tracker;

namespace lgo
{
    click_area_t::click_area_t(const click_area_t& in)
    :
        click_area_t
        {
            in.m_engine,
            in.m_depth_tracker,
            depth_range_t{ in.m_depth,in.m_depth },
            in.m_pos,
            in.m_size,
            in.m_callback_mouse_leave,
            in.m_callback_mouse_move,
            in.m_callback_mouse_left_click,
            in.m_callback_mouse_left_release,
            in.m_callback
        }
    {}

    click_area_t::click_area_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        size_2D size,
        callback_mouse_leave_t callback_mouse_leave,
        callback_mouse_move_t callback_mouse_move,
        callback_mouse_left_click_t callback_mouse_left_click,
        callback_mouse_left_release_t callback_mouse_left_release,
        callback_t callback
    )
    :
        m_engine{ engine },
        m_depth_tracker{ depth_tracker },
        m_pos{ pos },
        m_size{ size },
        m_depth{ depth_range.near },
        m_callback_mouse_leave{ callback_mouse_leave },
        m_callback_mouse_move{ callback_mouse_move },
        m_callback_mouse_left_click{ callback_mouse_left_click },
        m_callback_mouse_left_release{ callback_mouse_left_release },
        m_callback{ callback }
    {}

    auto click_area_t::operator=(const click_area_t& in) -> click_area_t&
    {
        m_engine = in.m_engine;
        m_depth_tracker = in.m_depth_tracker;
        m_pos = in.m_pos;
        m_size = in.m_size;
        m_depth = in.m_depth;
        m_callback_mouse_leave = in.m_callback_mouse_leave;
        m_callback_mouse_move = in.m_callback_mouse_move;
        m_callback_mouse_left_click = in.m_callback_mouse_left_click;
        m_callback_mouse_left_release = in.m_callback_mouse_left_release;
        m_callback = in.m_callback;
        return *this;
    }

    auto click_area_t::click_area_callback_mouse_enter(int entered) -> void
    {
        if (entered == GLFW_FALSE && m_inside)
        {
            std::unique_lock lock{ m_mutex };
            m_callback_mouse_leave();
            m_inside = false;
        }
        return;
    }

    auto click_area_t::click_area_callback_mouse_move(pos_2D pos) -> void
    {
        if (inside(m_pos, m_size, pos) && m_depth_tracker->top(pos, m_depth_id))
        {
            m_inside = true;
            if (glfwGetMouseButton(m_engine->window_get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
            {
                std::unique_lock lock{ m_mutex };
                m_callback_mouse_move(pos);
            }
        }
        else
        {
            if (m_inside)
            {
                std::unique_lock lock{ m_mutex };
                m_callback_mouse_leave();
                m_inside = false;
            }
            if (glfwGetMouseButton(m_engine->window_get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            {
                m_clicked_inside = false;
            }
        }
        return;
    }

    auto click_area_t::click_area_callback_mouse_button(int button, int action, int mods) -> void
    {
        if (button != GLFW_MOUSE_BUTTON_LEFT || !m_inside)
        {
            return;
        }
        pos_2D cursor_pos{ m_engine->cursor_pos_get() };
        if (action == GLFW_PRESS)
        {
            std::unique_lock lock{ m_mutex };
            m_callback_mouse_left_click(cursor_pos);
            m_clicked_inside = true;
        }
        else if (action == GLFW_RELEASE)
        {
            std::unique_lock lock{ m_mutex };
            m_callback_mouse_left_release(cursor_pos);
            if (m_clicked_inside)
            {
                auto callback_copy{ m_callback };
                lock.unlock();
                callback_copy();
            }
        }
        return;
    }

    auto click_area_t::get_pos() const -> pos_2D
    {
        std::unique_lock lock{ m_mutex };
        return m_pos;
    }

    auto click_area_t::set_pos(pos_2D pos) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_pos = pos;
        if (status() == shown)
        {
            m_depth_tracker->update(m_depth_id, m_pos, m_size);
        }
        return;
    }

    auto click_area_t::get_size() const -> size_2D
    {
        std::unique_lock lock{ m_mutex };
        return m_size;
    }

    auto click_area_t::set_size(size_2D size) -> void
    {
        GUI_object_t::set_size(size);
        std::unique_lock lock{ m_mutex };
        m_size = size;
        if (status() == shown)
        {
            m_depth_tracker->update(m_depth_id, m_pos, m_size);
        }
        return;
    }

    auto click_area_t::get_margin() const -> size_1D
    {
        return size_1D{ 0 };
    }

    auto click_area_t::log(std::uint32_t tab_count) const -> void
    {
        log_file(tab(tab_count) + "{\n");
        log_file(tab(tab_count + 1) + "type: click_area_t\n");
        GUI_object_t::log(tab_count + 1);
        log_file(tab(tab_count + 1) + "pos: " + to_string(m_pos) + "\n");
        log_file(tab(tab_count + 1) + "size: " + to_string(m_size) + "\n");
        log_file(tab(tab_count + 1) + "depth: " + std::to_string(m_depth) + "\n");
        log_file(tab(tab_count) + "}\n");
        return;
    }

    auto click_area_t::show_impl(bool base) -> void
    {
        if (!show_begin(base))
        {
            return;
        }
        std::unique_lock lock{ m_mutex };
        m_depth_id = m_depth_tracker->add(m_pos, m_size, m_depth, true);
        m_callback_mouse_enter_key = m_engine->callback_mouse_enter_add
        (
            std::bind(&click_area_t::click_area_callback_mouse_enter, this, std::placeholders::_1)
        );
        m_callback_mouse_move_key = m_engine->callback_mouse_move_add
        (
            std::bind(&click_area_t::click_area_callback_mouse_move, this, std::placeholders::_1)
        );
        m_callback_mouse_button_key = m_engine->callback_mouse_button_add
        (
            std::bind
            (
                &click_area_t::click_area_callback_mouse_button,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3
            )
        );
        click_area_callback_mouse_move(m_engine->get_cursor_pos());
        show_end(base);
        return;
    }

    auto click_area_t::hide_impl(bool base) -> void
    {
        if (!hide_begin(base))
        {
            return;
        }
        std::unique_lock lock{ m_mutex };
        m_engine->callback_mouse_enter_remove(m_callback_mouse_enter_key);
        m_engine->callback_mouse_move_remove(m_callback_mouse_move_key);
        m_engine->callback_mouse_button_remove(m_callback_mouse_button_key);
        m_depth_tracker->remove(m_depth_id);
        hide_end(base);
        return;
    }
}
