export module lgo.GUI.object.text_scroll;

import std;
import external.GLFW;
import lgo.io.file;
import lgo.math.vec;
import lgo.math.op;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.object.depth_tracker;
import lgo.GUI.object.object;
import lgo.GUI.object.rect;
import lgo.GUI.object.scroll;
import lgo.GUI.object.text;

export namespace lgo
{
    // A single text line has one horizontal viewport, independent of glyph
    // positions. The track keeps its original 2px thickness and reserves no Y bar.
    template<typename t_text>
    class text_scroll_impl_t: public t_text
    {
        size_2D m_size{};
        rect_t m_track{}, m_thumb{};
        depth_tracker_t* m_depth_tracker{};
        std::uint64_t m_depth_id{};
        float m_depth{};
        bool m_initialized{}, m_dragging{};
        std::int64_t m_grab_offset{};
        scroll_input_t m_input;
        static constexpr std::int64_t bar_height{2};
        static constexpr std::int64_t scroll_speed{32};

        auto max_scroll() const -> std::int64_t
        {
            return std::max<std::int64_t>(0, this->get_text_size().x - this->get_clip_size().x);
        }
        auto update_bar() -> void
        {
            const auto pos{get_pos()};
            const auto height{std::min(bar_height, m_size.y)};
            const auto y{pos.y + m_size.y - height};
            m_track.set_pos({pos.x, y});
            m_track.set_size({m_size.x, height});
            const auto content{std::max<std::int64_t>(1, this->get_text_size().x)};
            const auto width{max_scroll() ? std::clamp(m_size.x * m_size.x / content,
                std::min<std::int64_t>(24, m_size.x), m_size.x) : m_size.x};
            const auto offset{max_scroll() ? -this->get_text_offset().x * (m_size.x - width) / max_scroll() : 0};
            m_thumb.set_pos({pos.x + offset, y});
            m_thumb.set_size({width, height});
        }
        auto hit(pos_2D pos) const -> bool
        {
            return this->clip_contains(pos) && inside(get_pos(), m_size, pos) && m_depth_tracker->top(pos, m_depth_id);
        }
        auto drag(pos_2D pos) -> void
        {
            const auto travel{m_size.x - m_thumb.get_size().x};
            if (travel > 0)
            {
                const auto x{std::clamp(pos.x - get_pos().x - m_grab_offset, std::int64_t{}, travel)};
                set_scroll_offset(x * max_scroll() / travel);
            }
        }
      protected:
        auto text_layout_changed() -> void override
        {
            if (!m_initialized) return;
            set_scroll_offset(-this->get_text_offset().x);
        }
      public:
        text_scroll_impl_t() = default;
        template<typename... t_arg>
        text_scroll_impl_t(engine_t* engine, depth_tracker_t* tracker, depth_range_t depth,
                           pos_2D pos, std::string text, size_2D size, color_t color, t_arg&&... arg)
            : m_size{std::max<std::int64_t>(0, size.x), std::max<std::int64_t>(0, size.y)},
              m_depth_tracker{tracker}, m_depth{depth.near}
        {
            t_text::init(engine, tracker, depth, pos, std::move(text),
                size_2D{m_size.x, std::max<std::int64_t>(0, m_size.y - bar_height)},
                std::forward<t_arg>(arg)...);
            m_track = rect_t{engine, tracker, depth_range_t{(depth.near + depth.far) / 2, depth.far},
                             pos, size_2D{}, color_t{.15f,.15f,.15f}};
            m_thumb = rect_t{engine, tracker, depth, pos, size_2D{}, color};
            m_initialized = true;
            update_bar();
        }
        ~text_scroll_impl_t() override { this->hide(); }
        auto visit_children(const GUI_object_t::child_visitor_t& visit) -> void override
        { t_text::visit_children(visit); visit(m_track); visit(m_thumb); }
        auto get_pos() const -> pos_2D override { return t_text::get_pos(); }
        auto get_size() const -> size_2D override { return m_size; }
        auto get_margin() const -> size_1D override { return size_1D{0}; }
        auto scrollbar_pos() const -> pos_2D { return m_thumb.get_pos(); }
        auto scrollbar_size() const -> size_2D { return m_thumb.get_size(); }
        auto scroll_offset() const -> std::int64_t { return -this->get_text_offset().x; }
        auto set_scroll_offset(std::int64_t offset) -> void
        {
            std::unique_lock lock{this->m_mutex};
            this->set_text_offset({-std::clamp(offset, std::int64_t{}, max_scroll()), 0});
            update_bar();
        }
        auto set_pos(pos_2D pos) -> void override
        {
            std::unique_lock lock{this->m_mutex};
            t_text::set_pos(pos);
            update_bar();
            if (this->status() == this->shown) m_depth_tracker->update(m_depth_id, pos, m_size);
        }
        auto set_size(size_2D size) -> void override
        {
            std::unique_lock lock{this->m_mutex};
            size = {std::max<std::int64_t>(0, size.x), std::max<std::int64_t>(0, size.y)};
            GUI_object_t::set_size(size);
            m_size = size;
            this->resize_text({size.x, std::max<std::int64_t>(0, size.y - bar_height)});
            set_scroll_offset(scroll_offset());
            if (this->status() == this->shown) m_depth_tracker->update(m_depth_id, get_pos(), size);
        }
        auto set_text(std::string text, bool load_now = true) -> void
        {
            // Repeated status updates must not rebuild glyphs or reset scrolling.
            if (text == this->get_text()) return;
            t_text::set_text(std::move(text), load_now);
        }
        auto set_frame_color(color_t color) -> void { set_scrollbar_color(color); }
        auto set_scrollbar_color(color_t color) -> void { m_thumb.set_color(color); }
        auto log(std::uint32_t tab_count) const -> void override { t_text::log(tab_count); }
        auto show_impl(bool base) -> void override
        {
            if (!this->show_begin(base)) return;
            t_text::show_impl(false);
            m_track.show(); m_thumb.show();
            m_depth_id = m_depth_tracker->add(get_pos(), m_size, m_depth, false);
            m_input.attach(this->m_engine,this,m_depth,[this](pos_2D pos) { return hit(pos); },
                [this](size_2D delta)
                {
                    if (!delta.x) return false;
                    const auto old{scroll_offset()};
                    set_scroll_offset(old-scroll_speed*delta.x);
                    return old != scroll_offset();
                }, [this](pos_2D pos)
                { return max_scroll() && inside(m_track.get_pos(),m_track.get_size(),pos); },
                [this](pos_2D pos)
                {
                    m_dragging = true;
                    m_grab_offset = inside(m_thumb.get_pos(),m_thumb.get_size(),pos)
                        ? pos.x-m_thumb.get_pos().x : m_thumb.get_size().x/2;
                    drag(pos);
                }, [this](pos_2D pos) { drag(pos); }, [this] { m_dragging = false; });
            this->show_end(base);
        }
        auto hide_impl(bool base) -> void override
        {
            if (!this->hide_begin(base)) return;
            m_dragging = false;
            m_input.detach();
            m_depth_tracker->remove(m_depth_id);
            m_thumb.hide(); m_track.hide();
            t_text::hide_impl(false);
            this->hide_end(base);
        }
    };

    using text_line_scroll_t = text_scroll_impl_t<text_line_t>;
    class text_line_scroll_std_t: public text_line_scroll_t
    {
    public:
        static size_1D size_y;
        static size_1D font_size;
        static size_1D margin;

        text_line_scroll_std_t() = default;

        text_line_scroll_std_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            std::string text,
            color_t color,
            size_1D size_x,
            bool load_now = true
        );

        auto get_margin() const -> size_1D override;
    };

    /*class text_block_scroll_std_t: public text_block_scroll_t
    {
    public:
        static size_1D font_size;
        static size_1D margin;
        static color_t color;

        text_block_scroll_std_t();

        text_block_scroll_std_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            pos_2D pos,
            float depth,
            std::wstring text,
            size_2D size
        );

        auto get_margin() const -> size_1D override;
    };*/
}
