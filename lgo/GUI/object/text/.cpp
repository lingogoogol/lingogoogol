module lgo.GUI.object.text;

import std;

import external.Vulkan;

import lgo.container.str;
import lgo.io.file;
import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.Vulkan;
import lgo.GUI.primitive;
import lgo.GUI.texture;
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
    std::set<text_line_t *> text_line_t::m_live{};
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
        {
            std::lock_guard live_lock{m_load_mutex};
            m_live.insert(this);
        }
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
        {
            std::lock_guard live_lock{m_load_mutex};
            m_live.insert(this);
        }
        load_try(load_now);
        return;
    }
    
    text_line_t::~text_line_t()
    {
        //Always lock m_mutex before m_load_mutex to prevent deadlock.
        std::unique_lock lock{ m_mutex };
        std::unique_lock pending_lock{m_load_mutex};
        m_live.erase(this);
        m_pending.erase(this);
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
        // Parent layout needs the previous size when it computes the child's size delta.
        GUI_object_t::set_size(size);
        resize_text(size);
        return;
    }

    auto text_line_t::resize_text(size_2D size) -> void
    {
        std::unique_lock lock{ m_mutex };
        m_div.set_size(size);
        m_clip_size = size;
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
        if constexpr (!gui_object_names_enabled) return;
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
        std::unique_lock lock{ m_mutex };
        m_color = color;
        for (std::uint64_t id : m_rect_id)
        {
            m_div.get_object_shared<rect_t>(id)->set_color(color);
        }
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

    auto text_line_t::reload_fonts() -> void
    {
        std::vector<text_line_t *> texts;
        {
            std::lock_guard lock{m_load_mutex};
            texts.assign(m_live.begin(), m_live.end());
        }
        // Called on the GUI thread after replacing the shared font preference.
        // Old faces stay alive until all cached glyphs have been released.
        for (auto *text : texts)
        {
            std::lock_guard lock{text->m_mutex};
            text->load_try(false);
        }
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
            if (!ptr->m_engine->gpu_available()) return;
            m_pending.erase(begin);
            pending_lock.unlock();
            ptr->load();
        }
        return;
    }

    auto text_line_t::free() -> void
    {
        m_font.clear();
        return;
    }

    auto text_line_t::release_gpu() -> void
    {
        // Keep cache nodes and image-view addresses stable: visible rectangles reference them.
        for (auto& [face, glyphs] : m_font)
        {
            for (auto& [code, glyph] : glyphs)
            {
                glyph.m_image_view = {};
                glyph.m_image.clear();
            }
        }
    }

    auto text_line_t::upload_glyph(glyph_t& glyph, engine_t* engine, vk::raii::CommandBuffer& commands) -> void
    {
        pixmap_t empty{};
        empty.dim_set({ 1, 1 });
        empty[0, 0] = std::byte{ 0 };
        const auto& pixmap{ glyph.m_font.pixmap_get() };
        glyph.m_image.init
        ({
            .name{ "font glyph" },
            .device_physical{ &engine->device_physical_get() },
            .device{ &engine->device_get() },
            .command_buffer{ &commands },
            .pixmap{ pixmap.dim_get() == size_2D{ 0, 0 } ? &empty : &pixmap }
        });
        glyph.m_image_view = image_view_t
        {{
            .name{ "font glyph" },
            .device{ &engine->device_get() },
            .image{ &glyph.m_image.interface_get() },
            .format{ vk::Format::eR8Unorm },
            .aspect{ vk::ImageAspectFlagBits::eColor }
        }};
    }

    auto text_line_t::restore_gpu(engine_t* engine) -> void
    {
        auto gpu_guard{ engine->gpu_lock() };
        auto commands{ engine->command_buffer_create() };
        for (auto& [face, glyphs] : m_font)
        {
            for (auto& [code, glyph] : glyphs) upload_glyph(glyph, engine, commands);
        }
        commands.end();
        const vk::SubmitInfo submit{ .commandBufferCount{ 1 }, .pCommandBuffers{ &*commands } };
        engine->command_queue_get().submit(submit, nullptr);
        engine->command_queue_get().waitIdle();
    }

    auto text_line_t::load() -> void
    {
        m_engine->texture_recovery_set(&release_gpu, [engine = m_engine] { restore_gpu(engine); });
        if (m_engine->gpu_available())
        {
            try
            {
                load_gpu();
                std::unique_lock lock{ m_load_mutex };
                m_pending.erase(this);
                return;
            }
            catch (const vk::DeviceLostError&)
            {
                m_engine->device_lost();
            }
        }
        // Retry an interrupted glyph/layout update after device recovery. Never unwind
        // into HTTP completion bookkeeping or replay a user's upload/download action.
        std::unique_lock lock{ m_load_mutex };
        m_load_pending = true;
        m_pending.emplace(this);
    }

    auto text_line_t::load_gpu() -> void
    {
        std::unique_lock lock{ m_mutex };
        m_load_pending = false;
        m_div.clear_state();
        m_rect_id.clear();
        std::wstring text_w{ to_wstring(m_text) };
        // The pool, queue and uploaded resources are shared with frame rendering
        // and device recovery; keep the guard until this command buffer is freed.
        auto gpu_guard{ m_engine->gpu_lock() };
        vk::raii::CommandBuffer command_buffer{ m_engine->command_buffer_create() };
        m_glyph.resize(text_w.size());
        std::vector<font_face_t*> face_arr(text_w.size());
        for (std::size_t i{ 0 }; i < text_w.size(); ++i)
        {
            std::uint32_t index{};
            font_face_t* face{ m_font_preference->favorite(text_w[i], &index) };
            face_arr[i] = face;
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
                upload_glyph(glyph, m_engine, command_buffer);
                glyph.m_index = m_font_heap_index;
                ++m_font_heap_index;
                m_glyph[i] = &face_loaded.emplace(text_w[i], std::move(glyph)).first->second;
            }
            else
            {
                m_glyph[i] = &find_result->second;
            }
        }
        //Submit and wait for glyph image uploads to finish before adding `rect_t` objects to `m_div`:
        //if `m_div` is already shown, `m_div.add_object<rect_t>` immediately shows each `rect_t` and
        //registers its `rect_primitive_t` with `m_engine`, which requires `m_image` to be in `eShaderReadOnlyOptimal`.
        command_buffer.end();
        const vk::SubmitInfo submit_info
        {
            .commandBufferCount{ 1 },
            .pCommandBuffers{ &*command_buffer }
        };
        m_engine->command_queue_get().submit(submit_info, nullptr);
        m_engine->command_queue_get().waitIdle();

        m_rect_id.resize(text_w.size());
        for (std::size_t i{ 0 }; i < text_w.size(); ++i)
        {
            font_face_t* face{ face_arr[i] };
            //Since (0, 0) is at the top-left of the cell and +y points downward, the baseline is at
            //`y = face->ascender_get()` from the top of the cell, and `m_bbox_pos` is `(bitmap_left, -bitmap_top)`.
            m_rect_id[i] = m_div.add_object<rect_t>
            (
                m_depth_range,
                size_2D{ m_glyph[i]->m_font.advance_get().x, face->height_get() },
                m_clip_pos,
                m_clip_size,
                &m_glyph[i]->m_image_view,
                pos_2D{ 0, face->ascender_get() } + m_glyph[i]->m_font.bbox_pos_get(),
                size_2D{ m_glyph[i]->m_font.bbox_size_get().x, 0 },
                size_2D{ 0, m_glyph[i]->m_font.bbox_size_get().y },
                m_color
            );
        }
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
