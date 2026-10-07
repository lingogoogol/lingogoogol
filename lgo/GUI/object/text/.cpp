module lgo.GUI.object.text;

import std;

import external.Vulkan;

import lgo.container.str;
import lgo.io.file;
import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.Vulkan;
import lgo.GUI.primitive;
import lgo.GUI.font;
import lgo.GUI.object.object;
import lgo.GUI.object.depth_tracker;
import lgo.GUI.object.std;
import lgo.GUI.object.virt;
import lgo.GUI.object.div;
import lgo.GUI.object.rect;

namespace lgo
{
    std::mutex text_line_t::m_load_mutex{};
    std::set<text_line_t*> text_line_t::m_pending{};
    std::map<const font_face_t*, std::map<char32_t, text_line_t::glyph_t>> text_line_t::m_font{};
    std::uint64_t text_line_t::m_font_heap_index{ 0 };

    text_line_t::text_line_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        size_2D size,
        size_1D margin,
        const font_preference_t& font_preference,
        color_t color,
        alignment_2D alignment,
        bool load_now
    ):
        m_engine{ engine },
        m_div{ engine, depth_tracker, depth_range, pos, size, size_1D{ 0 }, alignment, false },
        m_depth_range{ depth_range },
        m_clip_pos{ pos },
        m_clip_size{ size },
        m_text{ text },
        m_font_preference{ &font_preference },
        m_margin{ margin },
        m_color{ color }
    {
        load_try(load_now);
        return;
    }

    auto text_line_t::init
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        size_2D size,
        size_1D margin,
        const font_preference_t& font_preference,
        color_t color,
        alignment_2D alignment,
        bool load_now
    )
    -> void
    {
        std::unique_lock lock{ m_mutex };
        m_engine = engine;
        m_div.init(engine, depth_tracker, depth_range, pos, size, size_1D{ 0 }, alignment, false);
        m_depth_range = depth_range;
        m_text = text;
        m_font_preference = &font_preference;
        m_color = color;
        m_margin = margin;
        m_clip_pos = pos;
        m_clip_size = size;
        load_try(load_now);
        return;
    }
    
    text_line_t::~text_line_t()
    {
        //Always lock m_mutex before m_load_mutex to prevent deadlock.
        std::unique_lock lock{ m_mutex };
        if (m_load_pending)
        {
            std::unique_lock pending_lock{ m_load_mutex };
            m_pending.erase(this);
        }
        return;
    }

    auto text_line_t::get_pos() const -> pos_2D
    {
        std::unique_lock lock{ m_mutex };
        return m_div.get_pos();
    }

    auto text_line_t::set_pos(pos_2D pos) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_clip_pos = m_clip_pos + (pos - m_div.get_pos());
        m_div.set_pos(pos);
        return;
    }

    auto text_line_t::get_size() const -> size_2D
    {
        std::unique_lock lock{ m_mutex };
        return m_div.get_size();
    }

    auto text_line_t::set_size(size_2D size) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_div.set_size(size);
        GUI_object_t::set_size(size);
        return;
    }

    auto text_line_t::get_margin() const -> size_1D
    {
        return m_margin;
    }

    auto text_line_t::depth_range_get() const -> depth_range_t
    {
        std::unique_lock lock{ m_mutex };
        return m_depth_range;
    }

    auto text_line_t::log(std::uint32_t tab_count) const -> void
    {
        log_file(tab(tab_count) + "{\n");
        log_file(tab(tab_count + 1) + "type: text_line_t\n");
        GUI_object_t::log(tab_count + 1);
        log_file(tab(tab_count + 1) + "depth_range: " + to_string(m_depth_range) + "\n");
        log_file(tab(tab_count + 1) + "text: " + m_text + "\n");
        log_file(tab(tab_count + 1) + "div:\n");
        m_div.log(tab_count + 1);
        log_file(tab(tab_count) + "}\n");
        return;
    }

    auto text_line_t::get_text() const -> const std::string&
    {
        std::unique_lock lock{ m_mutex };
        return m_text;
    }

    auto text_line_t::set_text(std::string text, bool load_now) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_text = text;
        load_try(load_now);
        return;
    }

    auto text_line_t::get_clip_pos() const -> pos_2D
    {
        std::unique_lock lock{ m_mutex };
        return m_clip_pos;
    }

    auto text_line_t::set_clip_pos(pos_2D clip_pos, bool load_now) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_clip_pos = clip_pos;
        load_try(load_now);
        return;
    }

    auto text_line_t::get_clip_size() const -> size_2D
    {
        std::unique_lock lock{ m_mutex };
        return m_clip_size;
    }

    auto text_line_t::set_clip_size(size_2D clip_size, bool load_now) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_clip_size = clip_size;
        load_try(load_now);
        return;
    }

    auto text_line_t::set_color(color_t color) -> void
    {
        m_color = color;
        //...
        return;
    }

    auto text_line_t::set_frame_color(color_t) -> void {}

    auto text_line_t::get_text_size() const -> size_2D
    {
        std::unique_lock lock{ m_mutex };
        return size_2D{ m_div.len().x, (*m_font_preference)[0]->leading_get() };
    }

    auto text_line_t::show_impl(bool base) -> void
    {
        if (!show_begin(base))
        {
            return;
        }
        std::unique_lock lock{ m_mutex };
        m_div.show();
        show_end(base);
        return;
    }

    auto text_line_t::hide_impl(bool base) -> void
    {
        if (!hide_begin(base))
        {
            return;
        }
        std::unique_lock lock{ m_mutex };
        m_div.hide();
        hide_end(base);
        return;
    }

    auto text_line_t::pending_load() -> void
    {
        while (true)
        {
            //Always lock m_mutex before m_load_mutex to prevent deadlock.
            std::unique_lock pending_lock{ m_load_mutex };
            std::set<text_line_t*>::iterator begin{}, end{};
            begin = m_pending.begin();
            end = m_pending.end();
            if (begin == end)
            {
                return;
            }
            text_line_t* ptr{ *begin };
            m_pending.erase(begin);
            pending_lock.unlock();
            ptr->load();
        }
        return;
    }

    auto text_line_t::load() -> void
    {
        std::unique_lock lock{ m_mutex };
        m_load_pending = false;
        m_div.clear_state();
        std::wstring text_w{ to_wstring(m_text) };
        vk::raii::CommandBuffer command_buffer{ m_engine->command_buffer_create() };
        m_glyph.resize(text_w.size());
        for (std::size_t i{ 0 }; i < text_w.size(); ++i)
        {
            std::uint32_t index{};
            font_face_t* face{ m_font_preference->favorite(text_w[i], &index) };
            if (!m_font.contains(face))
            {
                m_font.emplace(face, std::map<char32_t, glyph_t>{});
            }
            std::map<char32_t, glyph_t>& face_loaded{ m_font[face] };
            auto find_result{ face_loaded.find(text_w[i]) };
            if (find_result == face_loaded.end())
            {
                glyph_t glyph{};
                glyph.m_font.init(text_w[i], face, index);
                pixmap_t pixmap_empty{};//...move to init
                pixmap_empty.dim_set({ 1, 1 });
                pixmap_empty[0, 0] = std::byte{ 0 };
                const pixmap_t* pixmap_ptr{};
                if (glyph.m_font.pixmap_get().dim_get() == size_2D{ 0, 0 })
                {
                    pixmap_ptr = &pixmap_empty;
                }
                else
                {
                    pixmap_ptr = &glyph.m_font.pixmap_get();
                }
                glyph.m_image.init
                ({
                    .name
                    {
                        "text_line_t::m_font[family = " +
                        face->family_get() +
                        ", style = " + face->style_get() +
                        "][code = " +
                        std::to_string(static_cast<std::uint32_t>(text_w[i])) + "].m_image"
                    },
                    .device_physical{ &m_engine->device_physical_get() },
                    .device{ &m_engine->device_get() },
                    .command_buffer{ &command_buffer },
                    .pixmap{ pixmap_ptr }
                });
                glyph.m_index = m_font_heap_index;
                glyph.m_image_view = image_view_t
                { {
                    .device{ &m_engine->device_get() },
                    .image{ &glyph.m_image.interface_get() },
                    .format{ vk::Format::eR8Unorm },
                    .aspect{ vk::ImageAspectFlagBits::eColor }
                } };
                ++m_font_heap_index;
                m_glyph[i] = &face_loaded.emplace(text_w[i], std::move(glyph)).first->second;
            }
            else
            {
                m_glyph[i] = &find_result->second;
            }
            m_div.add_object<rect_t>
            (
                m_depth_range,
                size_2D{ m_glyph[i]->m_font.advance_get().x, face->height_get() >> 6 },
                m_clip_pos,
                m_clip_size,
                &m_glyph[i]->m_image_view,
                pos_2D{ 0, -face->descender_get() } + m_glyph[i]->m_font.bbox_pos_get(),
                size_2D{ m_glyph[i]->m_font.bbox_size_get().x, 0 },
                size_2D{ 0, m_glyph[i]->m_font.bbox_size_get().y }
            );
        }
        command_buffer.end();
        const vk::SubmitInfo submit_info
        {
            .commandBufferCount{ 1 },
            .pCommandBuffers{ &*command_buffer }
        };
        m_engine->command_queue_get().submit(submit_info, nullptr);
        m_engine->command_queue_get().waitIdle();
        return;
    }

    auto text_line_t::load_try(bool load_now) -> void
    {
        //Always lock m_mutex before m_load_mutex to prevent deadlock.
        std::unique_lock lock{ m_mutex };
        m_load_pending = !load_now;
        if (load_now)
        {
            load();
        }
        else
        {
            std::unique_lock pending_lock{ m_load_mutex };
            m_pending.emplace(this);
        }
        return;
    }

    size_1D text_line_std_t::margin{ std_margin };

    text_line_std_t::text_line_std_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        color_t color,
        size_1D size_x
    ):
        text_line_t
        {
            engine,
            depth_tracker,
            depth_range,
            pos,
            text,
            size_2D{ size_x.x, font_preference_t::default_get()[0]->leading_get() },
            margin,
            font_preference_t::default_get(),
            color,
            alignment_2D{ alignment_x::left, alignment_y::center }
        }
    {}

    text_line_flex_t::text_line_flex_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        size_1D size_x_min,
        size_1D size_y,
        size_1D margin,
        const font_preference_t& font_preference,
        color_t color,
        alignment_2D alignment
    )
    :
        text_line_t
        {
            engine,
            depth_tracker,
            depth_range,
            pos,
            "",
            size_2D{ 0, size_y.x },
            margin,
            font_preference,
            color,
            alignment
        },
        m_size_x_min{ size_x_min }
    {
        set_text(text);
        return;
    }

    auto text_line_flex_t::set_text(std::string text, bool load_now) -> void
    {
        std::unique_lock lock{ m_mutex };
        text_line_t::set_text(text, false);
        size_1D size_x{ std::max(get_text_size().x, m_size_x_min.x) };
        text_line_t::set_size(size_2D{ size_x.x, get_size().y });
        text_line_t::set_clip_size(size_2D{ size_x.x, get_clip_size().y }, load_now);
        return;
    }

    size_1D text_line_flex_std_t::margin{ std_margin };

    text_line_flex_std_t::text_line_flex_std_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        size_1D size_x_min,
        color_t color
    ):
        text_line_flex_t
        {
            engine,
            depth_tracker,
            depth_range,
            pos,
            text,
            size_x_min,
            std_forward,
            margin,
            font_preference_t::default_get(),
            color,
            alignment_2D{ alignment_x::left, alignment_y::center }
        }
    {}
}
