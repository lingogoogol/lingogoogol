module lgo.GUI.object.button;

import std;

import lgo.io.file;
import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.font;
import lgo.GUI.object.object;
import lgo.GUI.object.depth_tracker;
import lgo.GUI.object.rect_border;
import lgo.GUI.object.text;
import lgo.GUI.object.click_area;
import lgo.GUI.object.std;

namespace lgo
{
    button_classic_impl_t::button_classic_impl_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string display_text,
        size_2D size,
        color_t content_color,
        size_1D border_size,
        color_t border_color
    )
    :
        m_engine{ engine },
        m_rect
        {
            engine,
            depth_tracker,
            pos,
            size,
            content_color,
            border_size,
            border_color,
            depth_range_t{ (depth_range.near + depth_range.far) / 2, depth_range.far }
        },
        m_text
        {
            m_engine,
            depth_tracker,
            depth_range_t{ depth_range.near, (depth_range.near + depth_range.far) / 2 },
            m_rect.content_get_pos(),
            display_text,
            m_rect.content_get_size(),
            size_1D{ 0x10 },
            font_preference_t::default_get(),
            std_white,
            alignment_2D{ alignment_x::center, alignment_y::center }
        }
    {}

    auto button_classic_impl_t::get_pos() const -> pos_2D
    {
        return m_rect.get_pos();
    }

    auto button_classic_impl_t::set_pos(pos_2D pos) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_rect.set_pos(pos);
        m_text.set_pos(m_rect.content_get_pos());
        return;
    }

    auto button_classic_impl_t::get_size() const -> size_2D
    {
        return m_rect.get_size();
    }

    auto button_classic_impl_t::get_margin() const -> size_1D
    {
        return size_1D{ 0 };
    }

    auto button_classic_impl_t::log(std::uint32_t tab_count) const -> void
    {
        if constexpr (!gui_object_names_enabled) return;
        log_file(tab(tab_count) + "{\n");
        log_file(tab(tab_count + 1) + "type: button_classic_impl_t\n");
        GUI_object_t::log(tab_count + 1);
        log_file(tab(tab_count + 1) + "rect:\n");
        m_rect.log(tab_count + 1);
        log_file(tab(tab_count + 1) + "text:\n");
        m_text.log(tab_count + 1);
        log_file(tab(tab_count) + "}\n");
        return;
    }

    auto button_classic_impl_t::depth_range_get() const -> depth_range_t
    {
        return m_rect.depth_range_get();
    }

    auto button_classic_impl_t::button_enabled(bool enabled) -> void
    {
        m_rect.content_set_color(std_black);
        m_rect.border_set_color(enabled ? std_white : std_dark_gray);
        m_text.set_color(enabled ? std_white : std_dark_gray);
    }

    auto button_classic_impl_t::button_move(pos_2D) -> void
    {
        m_rect.border_set_color(std_yellow);
        m_text.set_color(std_yellow);
        return;
    }

    auto button_classic_impl_t::button_leave() -> void
    {
        m_rect.border_set_color(std_white);
        m_rect.content_set_color(std_black);
        m_text.set_color(std_white);
        return;
    }

    auto button_classic_impl_t::button_left_click(pos_2D) -> void
    {
        m_rect.border_set_color(std_white);
        m_rect.content_set_color(std_white);
        m_text.set_color(std_black);
        return;
    }

    auto button_classic_impl_t::button_left_release(pos_2D) -> void
    {
        m_rect.border_set_color(std_yellow);
        m_rect.content_set_color(std_black);
        m_text.set_color(std_yellow);
        return;
    }

    auto button_classic_impl_t::show_impl(bool base) -> void
    {
        if (!show_begin(base)) { return; }
        std::unique_lock lock{ m_mutex };
        m_rect.show();
        m_text.show();
        show_end(base);
        return;
    }

    auto button_classic_impl_t::hide_impl(bool base) -> void
    {
        if (!hide_begin(base))
        {
            return;
        }
        std::unique_lock lock{ m_mutex };
        m_text.hide();
        m_rect.hide();
        hide_end(base);
        return;
    }

    size_2D button_classic_std_impl_t::size{ std_sideways.x, std_forward.x };
    color_t button_classic_std_impl_t::content_color{ std_black };
    size_1D button_classic_std_impl_t::border_size{ std_line_size };
    color_t button_classic_std_impl_t::border_color{ std_white };
    size_1D button_classic_std_impl_t::margin{ std_margin };

    button_classic_std_impl_t::button_classic_std_impl_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string display_text
    )
    :
        button_classic_impl_t
        {
            engine,
            depth_tracker,
            depth_range,
            pos,
            display_text,
            size,
            content_color,
            size_1D{ 0x2 },
            border_color
        }
    {}

    auto button_classic_std_impl_t::get_margin() const -> size_1D
    {
        return margin;
    }
}
