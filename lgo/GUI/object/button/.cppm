export module lgo.GUI.object.button;

import std;

import lgo.io.file;
import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.depth_tracker;
import lgo.GUI.object.rect_border;
import lgo.GUI.object.text;
import lgo.GUI.object.click_area;
import lgo.GUI.object.std;

export namespace lgo
{
    struct button_tooltip_content_t
    {
        std::string text;
        pos_2D pos;
        size_2D size;
    };

    // One lazily-created overlay per engine, independent of button/scroll clips.
    // Providers are retained only while hovered and removed before owner teardown.
    class button_tooltip_t
    {
        struct impl_t;
        std::unique_ptr<impl_t> m_impl;
        explicit button_tooltip_t(engine_t* engine);
      public:
        ~button_tooltip_t();
        static auto shared(engine_t* engine) -> std::shared_ptr<button_tooltip_t>;
        auto activate(const void* owner,
            std::function<std::optional<button_tooltip_content_t>()> content) -> void;
        auto dismiss(const void* owner) -> void;
        auto snapshot() const -> std::optional<button_tooltip_content_t>;
        auto set_descriptions(std::function<std::string(std::string_view)> describe) -> void;
        auto describe(std::string_view label) const -> std::string;
    };

    template<typename t_type>
    class button_template_t: public GUI_object_t
    {
    private:
        engine_t* m_engine{};
        click_area_t m_click_area{};
        t_type* m_object{};
        std::shared_ptr<button_tooltip_t> m_tooltips;
        std::function<std::string()> m_tooltip;

        
        auto button_template_callback_mouse_leave() -> void;
        auto button_template_callback_mouse_move(pos_2D pos) -> void;
        auto button_template_callback_mouse_left_click(pos_2D pos) -> void;
        auto button_template_callback_mouse_left_release(pos_2D pos) -> void;

        template<typename u_type>
        friend class button_t;
    public:
        button_template_t() = default;
        ~button_template_t() override
        {
            hide();
            if (m_tooltips) m_tooltips->dismiss(this);
        }
        button_template_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            std::function<void(void)> callback,
            t_type* object
        );

        auto get_pos() const -> pos_2D override;
        auto set_pos(pos_2D pos) -> void override;
        auto get_size() const -> size_2D override;
        auto get_margin() const -> size_1D override;
        virtual auto log(std::uint32_t tab_count) const -> void override;

        auto visit_children(const child_visitor_t& visit) -> void override { visit(m_click_area); }
        auto show_impl(bool base) -> void override;
        auto hide_impl(bool base) -> void override;
    };

    template<typename t_type>
    auto button_template_t<t_type>::button_template_callback_mouse_leave() -> void
    {
        if (m_click_area.enabled()) m_object->button_leave();
        m_tooltips->dismiss(this);
        return;
    }

    template<typename t_type>
    auto button_template_t<t_type>::button_template_callback_mouse_move(pos_2D pos) -> void
    {
        if (m_click_area.enabled()) m_object->button_move(pos);
        m_tooltips->activate(this,[this]() -> std::optional<button_tooltip_content_t>
        {
            if (!m_click_area.hovered() || !m_tooltip) return std::nullopt;
            return button_tooltip_content_t{m_tooltip(),m_object->get_pos(),m_object->get_size()};
        });
        return;
    }

    template<typename t_type>
    auto button_template_t<t_type>::button_template_callback_mouse_left_click(pos_2D pos) -> void
    {
        m_tooltips->dismiss(this);
        m_object->button_left_click(pos);
        return;
    }

    template<typename t_type>
    auto button_template_t<t_type>::button_template_callback_mouse_left_release(pos_2D pos) -> void
    {
        m_object->button_left_release(pos);
        button_template_callback_mouse_move(pos);
        return;
    }

    template<typename t_type>
    button_template_t<t_type>::button_template_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        std::function<void(void)> callback,
        t_type* object
    )
    :
        m_engine{ engine },
        m_click_area
        {
            engine,
            depth_tracker,
            depth_range,
            object->get_pos(),
            object->get_size(),
            std::bind(&button_template_t<t_type>::button_template_callback_mouse_leave, this),
            std::bind(&button_template_t<t_type>::button_template_callback_mouse_move, this, std::placeholders::_1),
            std::bind(&button_template_t<t_type>::button_template_callback_mouse_left_click, this, std::placeholders::_1),
            std::bind(&button_template_t<t_type>::button_template_callback_mouse_left_release, this, std::placeholders::_1),
            callback
        },
        m_object{ object },
        m_tooltips{ button_tooltip_t::shared(engine) }
    {}

    template<typename t_type>
    auto button_template_t<t_type>::get_pos() const -> pos_2D
    {
        return m_click_area.get_pos();
    }

    template<typename t_type>
    auto button_template_t<t_type>::set_pos(pos_2D pos) -> void
    {
        m_click_area.set_pos(pos);
        return;
    }

    template<typename t_type>
    auto button_template_t<t_type>::get_size() const -> size_2D
    {
        return m_click_area.get_size();
    }

    template<typename t_type>
    auto button_template_t<t_type>::get_margin() const -> size_1D
    {
        return size_1D{ 0 };
    }

    template<typename t_type>
    auto button_template_t<t_type>::log(std::uint32_t tab_count) const -> void
    {
        if constexpr (!gui_object_names_enabled) return;
        GUI_object_t::log(tab_count);
        m_click_area.log(tab_count + 1);
    }

    template<typename t_type>
    auto button_template_t<t_type>::show_impl(bool base) -> void
    {
        if (!show_begin(base))
        {
            return;
        }
        std::unique_lock lock{ m_mutex };
        m_click_area.show();
        show_end(base);
        return;
    }

    template<typename t_type>
    auto button_template_t<t_type>::hide_impl(bool base) -> void
    {
        if (!hide_begin(base))
        {
            return;
        }
        std::unique_lock lock{ m_mutex };
        m_tooltips->dismiss(this);
        m_click_area.hide();
        hide_end(base);
        return;
    }

    template<typename t_type>
    class button_t: public t_type
    {
    private:
        button_template_t<t_type> m_button{};
        bool m_enabled{true};
        std::string m_tooltip, m_disabled_reason;
    public:
        button_t() = default;
        template<typename... u_arg>
        button_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            std::function<void(void)> callback,
            u_arg&&... arg
        );

        auto visit_children(const GUI_object_t::child_visitor_t& visit) -> void override
        { t_type::visit_children(visit); visit(m_button); }
        auto set_enabled(bool enabled, std::string reason = {}) -> void;
        auto set_tooltip(std::string text) -> void { m_tooltip = std::move(text); }
        auto tooltip_text() const -> std::string
        {
            auto text = m_tooltip.empty() ? m_button.m_tooltips->describe(t_type::button_label()) : m_tooltip;
            if (!m_enabled) text += "\n不可用：" + (m_disabled_reason.empty() ? std::string{"目前狀態不允許此操作"} : m_disabled_reason);
            return text;
        }
        auto enabled() const -> bool { return m_enabled; }
        auto set_pos(pos_2D pos) -> void override;
        auto set_size(size_2D size) -> void override;
        virtual auto log(std::uint32_t tab_count) const -> void override;

        auto show_impl(bool base) -> void override;
        auto hide_impl(bool base) -> void override;
    };

    template<typename t_type>
    template<typename... u_arg>
    button_t<t_type>::button_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::function<void(void)> callback,
        u_arg&&... arg
    )
    :
        t_type{ engine, depth_tracker, depth_range, pos, std::forward<u_arg>(arg)... },
        m_button{ engine, depth_tracker, depth_range,
            [this, callback = std::move(callback)] { if (m_enabled) callback(); }, static_cast<t_type*>(this) }
    { m_button.m_tooltip = [this] { return tooltip_text(); }; }

    template<typename t_type>
    auto button_t<t_type>::set_enabled(bool enabled, std::string reason) -> void
    {
        std::unique_lock lock{this->m_mutex};
        m_disabled_reason = std::move(reason);
        if (m_enabled == enabled) return;
        m_enabled = enabled;
        // Keep hit testing alive for disabled tooltips; clicks and pressed
        // state are suppressed independently of hover and rendering.
        m_button.m_click_area.set_enabled(enabled);
        t_type::button_enabled(enabled);
    }

    template<typename t_type>
    auto button_t<t_type>::set_pos(pos_2D pos) -> void
    {
        std::unique_lock lock{ this->m_mutex };
        t_type::set_pos(pos);
        m_button.set_pos(pos);
        return;
    }

    template<typename t_type>
    auto button_t<t_type>::set_size(size_2D size) -> void
    {
        std::unique_lock lock{ this->m_mutex };
        t_type::set_size(size);
        m_button.m_click_area.set_size(size);
    }

    template<typename t_type>
    auto button_t<t_type>::log(std::uint32_t tab_count) const -> void
    {
        if constexpr (!gui_object_names_enabled) return;
        log_file(tab(tab_count) + "{\n");
        log_file(tab(tab_count + 1) + "type: button_t<>\n");
        t_type::log(tab_count + 1);
        log_file(tab(tab_count + 1) + "click_area:\n");
        m_button.m_click_area.log(tab_count + 1);
        log_file(tab(tab_count) + "}\n");
        return;
    }

    template<typename t_type>
    auto button_t<t_type>::show_impl(bool base) -> void
    {
        if (!this->show_begin(base))
        {
            return;
        }
        std::unique_lock lock{ this->m_mutex };
        m_button.show();
        t_type::show_impl(false);
        this->show_end(base);
        return;
    }

    template<typename t_type>
    auto button_t<t_type>::hide_impl(bool base) -> void
    {
        if (!this->hide_begin(base))
        {
            return;
        }
        std::unique_lock lock{ this->m_mutex };
        t_type::hide_impl(false);
        m_button.hide();
        this->hide_end(base);
        return;
    }

    template<typename t_text>
    class button_text_impl_t: public t_text
    {
    private:
        color_t m_color{};
    public:
        button_text_impl_t() = default;
        template<typename... u_arg>
        button_text_impl_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            std::string text,
            color_t color,
            u_arg&&... arg
        );

        auto depth_range_get() const -> depth_range_t;

        auto button_label() const -> std::string { return t_text::get_text(); }
        auto button_enabled(bool enabled) -> void;
        auto button_move(pos_2D) -> void;
        auto button_leave() -> void;
        auto button_left_click(pos_2D) -> void;
        auto button_left_release(pos_2D) -> void;
    };

    template<typename t_text>
    template<typename... u_arg>
    button_text_impl_t<t_text>::button_text_impl_t
    (
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        color_t color,
        u_arg&&... arg
    )
    :
        t_text{ engine, depth_tracker, depth_range, pos, text, color, std::forward<u_arg>(arg)... },
        m_color{ color }
    {}

    template<typename t_text>
    auto button_text_impl_t<t_text>::depth_range_get() const -> depth_range_t
    {
        return t_text::depth_range_get();
    }

    template<typename t_text>
    auto button_text_impl_t<t_text>::button_enabled(bool enabled) -> void
    {
        t_text::set_color(enabled ? m_color : std_dark_gray);
    }

    template<typename t_text>
    auto button_text_impl_t<t_text>::button_move(pos_2D) -> void
    {
        t_text::set_color(std_yellow);
        return;
    }

    template<typename t_text>
    auto button_text_impl_t<t_text>::button_leave() -> void
    {
        t_text::set_color(m_color);
        return;
    }

    template<typename t_text>
    auto button_text_impl_t<t_text>::button_left_click(pos_2D) -> void
    {
        t_text::set_color(std_white);
        return;
    }

    template<typename t_text>
    auto button_text_impl_t<t_text>::button_left_release(pos_2D) -> void
    {
        t_text::set_color(std_yellow);
        return;
    }

    template<typename t_text>
    using button_text_t = button_t<button_text_impl_t<t_text>>;

    class button_classic_impl_t: public GUI_object_t
    {
    private:
        engine_t* m_engine{};
        rect_border_t m_rect{};
        text_line_t m_text{};
    public:
        button_classic_impl_t() = default;
        button_classic_impl_t
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
        );

        auto get_pos() const -> pos_2D override;
        auto set_pos(pos_2D pos) -> void override;
        auto get_size() const -> size_2D override;
        auto get_margin() const -> size_1D override;
        auto depth_range_get() const -> depth_range_t;
        virtual auto log(std::uint32_t tab_count) const -> void override;
        
        auto visit_children(const child_visitor_t& visit) -> void override { visit(m_rect); visit(m_text); }
        auto button_label() const -> std::string { return m_text.get_text(); }
        auto button_enabled(bool enabled) -> void;
        auto button_move(pos_2D) -> void;
        auto button_leave() -> void;
        auto button_left_click(pos_2D) -> void;
        auto button_left_release(pos_2D) -> void;

        auto show_impl(bool base) -> void override;
        auto hide_impl(bool base) -> void override;
    };

    class button_classic_std_impl_t: public button_classic_impl_t
    {
    public:
        static size_2D size;
        static color_t content_color;
        static size_1D border_size;
        static color_t border_color;
        static size_1D margin;

        button_classic_std_impl_t() = default;
        button_classic_std_impl_t
        (
            engine_t* engine,
            depth_tracker_t* depth_tracker,
            depth_range_t depth_range,
            pos_2D pos,
            std::string display_text
        );

        auto get_margin() const -> size_1D override;
    };

    using button_classic_t = button_t<button_classic_impl_t>;
    using button_classic_std_t = button_t<button_classic_std_impl_t>;
}
