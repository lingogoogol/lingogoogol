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
            in.m_mouse_move_callback,
            in.m_mouse_leave_callback,
            in.m_mouse_left_click_callback,
            in.m_mouse_left_release_callback,
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
        std::function<void(pos_2D)> mouse_move_callback,
        std::function<void(void)> mouse_leave_callback,
        std::function<void(pos_2D)> mouse_left_click_callback,
        std::function<void(pos_2D)> mouse_left_release_callback,
        std::function<void(void)> callback
    )
    :
        m_engine{ engine },
        m_depth_tracker{ depth_tracker },
        m_pos{ pos },
        m_size{ size },
        m_depth{ depth_range.near },
        m_mouse_move_callback{ mouse_move_callback },
        m_mouse_leave_callback{ mouse_leave_callback },
        m_mouse_left_click_callback{ mouse_left_click_callback },
        m_mouse_left_release_callback{ mouse_left_release_callback },
        m_callback{ callback }
    {}

    auto click_area_t::operator=(const click_area_t& in) -> click_area_t&
    {
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

    auto click_area_t::click_area_mouse_move_callback(pos_2D pos) -> void
    {
        if (inside(m_pos, m_size, pos) && m_depth_tracker->top(pos, m_depth_id))
        {
            m_inside = true;
            if (glfwGetMouseButton(m_engine->window_get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
            {
                std::unique_lock lock{ m_mutex };
                m_mouse_move_callback(pos);
            }
        }
        else
        {
            if (m_inside)
            {
                std::unique_lock lock{ m_mutex };
                m_mouse_leave_callback();
                m_inside = false;
            }
            if (glfwGetMouseButton(m_engine->window_get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            {
                m_clicked_inside = false;
            }
        }
        return;
    }

    auto click_area_t::click_area_mouse_leave_callback() -> void
    {
        if (m_inside)
        {
            std::unique_lock lock{ m_mutex };
            m_mouse_leave_callback();
            m_inside = false;
        }
        return;
    }

    auto click_area_t::click_area_mouse_left_click_callback(pos_2D pos) -> void
    {
        if (m_inside)
        {
            std::unique_lock lock{ m_mutex };
            m_mouse_left_click_callback(pos);
            m_clicked_inside = true;
        }
        return;
    }

    auto click_area_t::click_area_mouse_left_release_callback(pos_2D pos) -> void
    {
        if (m_inside)
        {
            std::unique_lock lock{ m_mutex };
            m_mouse_left_release_callback(pos);
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
        m_mouse_move_callback_key = m_engine->add_mouse_move
        (
            std::bind(&click_area_t::click_area_mouse_move_callback, this, std::placeholders::_1)
        );
        m_mouse_leave_callback_key = m_engine->add_mouse_leave
        (
            std::bind(&click_area_t::click_area_mouse_leave_callback, this)
        );
        m_mouse_left_click_callback_key = m_engine->add_mouse_left_click
        (
            std::bind(&click_area_t::click_area_mouse_left_click_callback, this, std::placeholders::_1)
        );
        m_mouse_left_release_callback_key = m_engine->add_mouse_left_release
        (
            std::bind(&click_area_t::click_area_mouse_left_release_callback, this, std::placeholders::_1)
        );
        click_area_mouse_move_callback(m_engine->get_cursor_pos());
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
        m_engine->remove_mouse_move(m_mouse_move_callback_key);
        m_engine->remove_mouse_leave(m_mouse_leave_callback_key);
        m_engine->remove_mouse_left_click(m_mouse_left_click_callback_key);
        m_engine->remove_mouse_left_release(m_mouse_left_release_callback_key);
        m_depth_tracker->remove(m_depth_id);
        hide_end(base);
        return;
    }
}
