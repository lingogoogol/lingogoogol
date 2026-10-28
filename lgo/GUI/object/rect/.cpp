module lgo.GUI.object.rect;

import std;

import lgo.math.vec;
import lgo.math.op;
import lgo.io.file;
import lgo.GUI.misc;
import lgo.GUI.Vulkan;
import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.std;
import lgo.GUI.object.depth_tracker;

namespace lgo
{
    rect_t::rect_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        size_2D size
    )
    :
        rect_t
        {
            engine,
            depth_tracker,
            depth_range,
            pos,
            size,
            pos,
            size
        }
    {}

    rect_t::rect_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        size_2D size,
        pos_2D clip_pos,
        size_2D clip_size
    )
    :
        m_engine{ engine },
        m_depth_tracker{ depth_tracker },
        m_pos{ pos },
        m_size{ size },
        m_depth_range{ depth_range },
        m_clip_pos{ clip_pos },
        m_clip_size{ clip_size }
    {}

    rect_t::rect_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        size_2D size,
        color_t color
    )
    :
        rect_t
        {
            engine,
            depth_tracker,
            depth_range,
            pos,
            size
        }
    {
        m_color = color;
        m_texture_enable = false;
        return;
    }

    rect_t::rect_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        size_2D size,
        image_view_t* image_view
    )
    :
        rect_t
        {
            engine,
            depth_tracker,
            depth_range,
            pos,
            size,
            pos,
            size,
            image_view,
            pos,
            size_2D{ size.x, 0 },
            size_2D{ 0, size.y }
        }
    {}

    rect_t::rect_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        size_2D size,
        pos_2D clip_pos,
        size_2D clip_size,
        image_view_t* image_view,
        pos_2D texture_pos,
        size_2D texture_axis_x,
        size_2D texture_axis_y,
        color_t color
    )
    :
        rect_t
        {
            engine,
            depth_tracker,
            depth_range,
            pos,
            size,
            clip_pos,
            clip_size
        }
    {
        m_color = color;
        m_image_view = image_view;
        m_texture_pos = texture_pos;
        m_texture_axis_x = texture_axis_x;
        m_texture_axis_y = texture_axis_y;
        m_texture_enable = true;
        m_clip_enabled = true;
        return;
    }

    auto rect_t::get_pos() const -> pos_2D
    {
        std::unique_lock lock{ m_mutex };
        return m_pos;
    }

    auto rect_t::set_pos(pos_2D pos) -> void
    {
        std::unique_lock lock{ m_mutex };
        size_2D offset{ pos - m_pos };
        m_clip_pos = m_clip_pos + offset;
        m_texture_pos = m_texture_pos + offset;
        m_pos = pos;
        if (m_primitive)
        {
            m_primitive->set_pos(pos);
            update_hit_area();
        }
        return;
    }

    auto rect_t::set_clip(pos_2D pos, size_2D size) -> void
    {
        std::unique_lock lock{m_mutex};
        m_clip_pos = pos;
        m_clip_size = size;
        m_clip_enabled = true;
        if (m_primitive)
        {
            m_primitive->set_clip(&m_clip_pos, &m_clip_size);
            update_hit_area();
        }
    }

    auto rect_t::clip_changed() -> void
    {
        auto bounds{clip_bounds({m_pos, m_size})};
        if (m_clip_enabled) bounds = bounds.intersect({m_clip_pos, m_clip_size});
        m_render_clip_pos = bounds.pos;
        m_render_clip_size = bounds.size;
        if (m_primitive)
        {
            m_primitive->set_clip(&m_render_clip_pos, &m_render_clip_size);
            m_depth_tracker->update(m_depth_id, bounds.pos, bounds.size);
        }
    }

    auto rect_t::update_hit_area() -> void { clip_changed(); }

    auto rect_t::get_size() const -> size_2D
    {
        std::unique_lock lock{ m_mutex };
        return m_size;
    }

    auto rect_t::set_size(size_2D size) -> void
    {
        std::unique_lock lock{ m_mutex };
        GUI_object_t::set_size(size);
        m_size = size;
        if (m_primitive)
        {
            m_primitive->set_size(size);
            update_hit_area();
        }
        return;
    }
    
    auto rect_t::log(std::uint32_t tab_count) const -> void
    {
        if constexpr (!gui_object_names_enabled) return;
        log_file(tab(tab_count) + "{\n");
        log_file(tab(tab_count + 1) + "type: rect_t\n");
        GUI_object_t::log(tab_count + 1);
        log_file(tab(tab_count + 1) + "pos: " + to_string(m_pos) + "\n");
        log_file(tab(tab_count + 1) + "size: " + to_string(m_size) + "\n");
        log_file(tab(tab_count + 1) + "depth_range: " + to_string(m_depth_range) + "\n");
        log_file(tab(tab_count) + "}\n");
        return;
    }

    auto rect_t::get_margin() const -> size_1D
    {
        return size_1D{ 0 };
    }

    auto rect_t::set_color(color_t color) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_color = color;
        if (m_primitive)
        {
            m_primitive->set_color(color);
        }
        return;
    }

    auto rect_t::set_image_view(image_view_t* image_view) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_image_view = image_view;
        if (m_primitive)
        {
            m_primitive->set_image_view(*image_view);
        }
        return;
    }

    auto rect_t::color_enable() -> void
    {
        std::unique_lock lock{ m_mutex };
        m_texture_enable = false;
        if (m_primitive)
        {
            m_primitive->color_enable();
        }
        return;
    }

    auto rect_t::texture_enable() -> void
    {
        std::unique_lock lock{ m_mutex };
        m_texture_enable = true;
        if (m_primitive)
        {
            m_primitive->texture_enable();
        }
        return;
    }

    auto rect_t::inside(pos_2D pos) -> bool
    {
        std::unique_lock lock{ m_mutex };
        return ::lgo::inside(m_pos, m_size, pos);
    }

    auto rect_t::show_impl(bool base) -> void
    {
        if (!show_begin(base))
        {
            return;
        }
        std::unique_lock lock{ m_mutex };
        auto engine{ m_engine };
        auto pos{ m_pos };
        auto size{ m_size };
        auto depth_range{ m_depth_range };
        auto color{ m_color };
        lock.unlock();
        clip_changed();
        rect_primitive_t* primitive{};
        if (m_texture_enable)
        {
            primitive = engine->add_rect
            ({
                .pos{ pos },
                .size{ size },
                .depth{ depth_range.near },
                .clip_pos{&m_render_clip_pos},
                .clip_size{&m_render_clip_size},
                .color{ color },
                .image_view{ m_image_view },
                .texture_pos{ m_texture_pos },
                .texture_axis_x{ m_texture_axis_x },
                .texture_axis_y{ m_texture_axis_y },
                .name{ m_name + ".m_primitive" }
            });
        }
        else
        {
            primitive = engine->add_rect
            ({
                .pos{ pos },
                .size{ size },
                .depth{ depth_range.near },
                .clip_pos{&m_render_clip_pos},
                .clip_size{&m_render_clip_size},
                .color{ color },
                .name{ m_name + ".m_primitive" }
            });
        }
        lock.lock();
        m_primitive = primitive;
        m_depth_id = m_depth_tracker->add(m_pos, m_size, m_depth_range.near, true);
        update_hit_area();
        show_end(base);
        return;
    }

    auto rect_t::hide_impl(bool base) -> void
    {
        if (!hide_begin(base))
        {
            return;
        }
        std::unique_lock lock{ m_mutex };
        auto engine{ m_engine };
        auto primitive{ m_primitive };
        m_primitive = nullptr;
        m_depth_tracker->remove(m_depth_id);
        lock.unlock();
        engine->remove_rect(primitive);
        hide_end(base);
        return;
    }
    progress_bar_t::progress_bar_t(engine_t* engine, depth_tracker_t* tracker, depth_range_t depth,
                                  pos_2D pos, size_2D size, color_t color)
        : m_track{engine, tracker, depth_range_t{(depth.near + depth.far) / 2, depth.far},
                  pos, size, color_t{.12f,.12f,.12f}},
          m_fill{engine, tracker, depth, pos, size_2D{0,size.y}, color}, m_size{size}
    {}

    auto progress_bar_t::set_pos(pos_2D pos) -> void
    {
        m_track.set_pos(pos); m_fill.set_pos(pos);
    }
    auto progress_bar_t::set_size(size_2D size) -> void
    {
        size = {std::max<std::int64_t>(0,size.x), std::max<std::int64_t>(0,size.y)};
        GUI_object_t::set_size(size);
        m_size = size;
        m_track.set_size(size);
        set_fraction(m_fraction);
    }
    auto progress_bar_t::set_fraction(double fraction) -> void
    {
        m_fraction = std::isfinite(fraction) ? std::clamp(fraction, 0., 1.) : 0.;
        m_fill.set_size({static_cast<std::int64_t>(m_fraction * m_size.x), m_size.y});
    }
    auto progress_bar_t::show_impl(bool base) -> void
    {
        if (!show_begin(base)) return;
        m_track.show(); m_fill.show(); show_end(base);
    }
    auto progress_bar_t::hide_impl(bool base) -> void
    {
        if (!hide_begin(base)) return;
        m_fill.hide(); m_track.hide(); hide_end(base);
    }

}
