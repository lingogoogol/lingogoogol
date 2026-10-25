module lgo.GUI.object.rect_border;

import std;

import lgo.io.file;
import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.std;
import lgo.GUI.object.depth_tracker;
import lgo.GUI.object.rect;

namespace lgo
{
    size_1D rect_border_t::margin{ std_margin };

    auto rect_border_t::compute_inner_pos() -> pos_2D {
        std::unique_lock lock{ m_mutex };
        return pos_2D{ m_outer_rect.get_pos().x + m_border_size.x, m_outer_rect.get_pos().y + m_border_size.x };
    }

    auto rect_border_t::compute_inner_size() -> size_2D {
        std::unique_lock lock{ m_mutex };
        return size_2D{ m_outer_rect.get_size().x - m_border_size.x * 2, m_outer_rect.get_size().y - m_border_size.x * 2 };
    }

    rect_border_t::rect_border_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        pos_2D pos,
        size_2D size,
        color_t content_color,
        size_1D border_size,
        color_t border_color,
        depth_range_t depth_range
    )
    :
        m_border_size{ border_size },
        m_depth_range{ depth_range }
    {
        float depth_distance{ (m_depth_range.far - m_depth_range.near) / 2 };
        m_outer_rect = rect_t
        {
            engine,
            depth_tracker,
            depth_range_t{ m_depth_range.near + depth_distance, m_depth_range.far },
            pos,
            size,
            border_color
        };
        m_inner_rect = rect_t
        {
            engine,
            depth_tracker,
            depth_range_t{ m_depth_range.near, m_depth_range.near + depth_distance },
            compute_inner_pos(),
            compute_inner_size(),
            content_color
        };
        return;
    }

    auto rect_border_t::get_pos() const -> pos_2D
    {
        return m_outer_rect.get_pos();
    }

    auto rect_border_t::set_pos(pos_2D pos) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_outer_rect.set_pos(pos);
        m_inner_rect.set_pos(compute_inner_pos());
        return;
    }

    auto rect_border_t::get_size() const -> size_2D
    {
        return m_outer_rect.get_size();
    }

    auto rect_border_t::set_size(size_2D size) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_outer_rect.set_size(size);
        m_inner_rect.set_size(compute_inner_size());
        return;
    }

    auto rect_border_t::get_border_size() const -> size_1D
    {
        std::unique_lock lock{ m_mutex };
        return m_border_size;
    }

    auto rect_border_t::get_margin() const -> size_1D
    {
        return margin;
    }

    auto rect_border_t::depth_range_get() const -> depth_range_t
    {
        std::unique_lock lock{ m_mutex };
        return m_depth_range;
    }

    auto rect_border_t::log(std::uint32_t tab_count) const -> void
    {
        if constexpr (!gui_object_names_enabled) return;
        log_file(tab(tab_count) + "{\n");
        log_file(tab(tab_count + 1) + "type: rect_border_t\n");
        GUI_object_t::log(tab_count + 1);
        log_file(tab(tab_count + 1) + "outer_rect:\n");
        m_outer_rect.log(tab_count + 1);
        log_file(tab(tab_count + 1) + "inner_rect:\n");
        m_inner_rect.log(tab_count + 1);
        log_file(tab(tab_count) + "}\n");
        return;
    }

    auto rect_border_t::border_set_color(color_t color) -> void
    {
        m_outer_rect.set_color(color);
        return;
    }

    auto rect_border_t::content_get_pos() -> pos_2D
    {
        return m_inner_rect.get_pos();
    }

    auto rect_border_t::content_get_size() -> size_2D
    {
        return m_inner_rect.get_size();
    }

    auto rect_border_t::content_set_color(color_t color) -> void
    {
        m_inner_rect.set_color(color);
        return;
    }

    auto rect_border_t::inside(pos_2D pos) -> bool
    {
        return m_outer_rect.inside(pos);
    }

    auto rect_border_t::show_impl(bool base) -> void
    {
        if (!show_begin(base))
        {
            return;
        }
        std::unique_lock lock{ m_mutex };
        m_outer_rect.show();
        m_inner_rect.show();
        show_end(base);
        return;
    }

    auto rect_border_t::hide_impl(bool base) -> void
    {
        if (!hide_begin(base))
        {
            return;
        }
        std::unique_lock lock{ m_mutex };
        m_inner_rect.hide();
        m_outer_rect.hide();
        hide_end(base);
        return;
    }

    color_t std_border_t::content_color{ std_black };
    size_1D std_border_t::border_size{ std_line_size };
    size_1D std_border_t::margin{ std_margin };

    std_border_t::std_border_t()
    :
        std_border_t
        {
            nullptr,
            nullptr,
            pos_2D{ 0, 0 },
            size_2D{ 0, 0 },
            std_white,
            depth_range_t{ 0.0f, 1.0f }
        }
    {}

    std_border_t::std_border_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        pos_2D pos,
        size_2D size,
        color_t border_color,
        depth_range_t depth_range
    )
    :
        rect_border_t
        {
            engine,
            depth_tracker,
            pos,
            size,
            content_color,
            border_size,
            border_color,
            depth_range
        }
    {}
}
