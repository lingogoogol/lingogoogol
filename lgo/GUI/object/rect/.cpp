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
        size_2D texture_axis_y
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
        m_image_view = image_view;
        m_texture_pos = texture_pos;
        m_texture_axis_x = texture_axis_x;
        m_texture_axis_y = texture_axis_y;
        m_texture_enable = true;
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
        m_clip_pos = m_clip_pos + (pos - m_pos);
        m_pos = pos;
        if (m_primitive)
        {
            m_primitive->set_pos(pos);
        }
        return;
    }

    auto rect_t::get_size() const -> size_2D
    {
        std::unique_lock lock{ m_mutex };
        return m_size;
    }

    auto rect_t::set_size(size_2D size) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_size = size;
        if (m_primitive)
        {
            m_primitive->set_size(size);
        }
        return;
    }
    
    auto rect_t::log(std::uint32_t tab_count) const -> void
    {
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
        rect_primitive_t* primitive{};
        if (m_texture_enable)
        {
            primitive = engine->add_rect
            ({
                .pos{ pos },
                .size{ size },
                .depth{ depth_range.near },
                .clip_pos{},//...
                .clip_size{},//...
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
                .color{ color },
                .name{ m_name + ".m_primitive" }
            });
        }
        lock.lock();
        m_primitive = primitive;
        m_depth_id = m_depth_tracker->add(m_pos, m_size, m_depth_range.near, true);
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
}
