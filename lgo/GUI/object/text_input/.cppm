export module lgo.GUI.object.text_input;

import std;
import external.GLFW;

import lgo.io.file;
import lgo.math.vec;
import lgo.GUI.misc;
import lgo.GUI.primitive;
import lgo.GUI.object.object;
import lgo.GUI.object.depth_tracker;
import lgo.GUI.object.text_scroll;
import lgo.GUI.object.click_area;
import lgo.GUI.object.std;
import lgo.GUI.object.focus;
import lgo.GUI.object.text;

export namespace lgo
{
    template <typename t_text> class text_input_constraint_impl_t : public t_text
    {
      private:
        engine_t* m_engine{};
        focus_t* m_focus{};
        std::atomic_bool m_on_focus{false};
        click_area_t m_click_area{};
        std::function<void(char32_t)>* m_callback{};
        engine_t::callback_key_t* m_key_callback{};
        bool m_select_all{};
        auto text_input_key_callback(int key, int action, int mods) -> void;
        std::function<void(std::string*, char32_t)> m_constraint{};

        auto text_input_cha_callback(char32_t in) -> void;
        auto text_input_click_area_callback() -> void;
        auto text_input_on_focus_callback() -> void;

        auto set_callback() -> void;
        auto clear_callback() -> void;

      public:
        static auto constraint_default(std::string* text, char32_t in) -> void;
        static auto only_num(std::string* text, char32_t in) -> void;
        static auto limit_len(std::string* text, char32_t in, std::size_t len) -> void;
        static auto only_num_limit_len(std::string* text, char32_t in, std::size_t len) -> void;

        text_input_constraint_impl_t() = default;

        template <typename... t_arg>
        text_input_constraint_impl_t(engine_t* engine, depth_tracker_t* depth_tracker,
                                     depth_range_t depth_range, pos_2D pos, std::string text,
                                     std::function<void(std::string*, char32_t)> constraint, focus_t* focus,
                                     t_arg&&... arg);

        auto set_pos(pos_2D pos) -> void override;
        virtual auto log(std::uint32_t tab_count) const -> void override;

        auto visit_children(const GUI_object_t::child_visitor_t& visit) -> void override
        { t_text::visit_children(visit); visit(m_click_area); }
        auto show_impl(bool base) -> void override;
        auto hide_impl(bool base) -> void override;
    };

    template <typename t_text>
    auto text_input_constraint_impl_t<t_text>::text_input_cha_callback(char32_t in) -> void
    {
        std::unique_lock lock{this->m_mutex};
        std::string text{m_select_all ? std::string{} : t_text::get_text()};
        m_select_all = false;
        if (m_constraint)
        {
            m_constraint(&text, in);
        }
        else
        {
            constraint_default(&text, in);
        }
        t_text::set_text(text);
        m_click_area.set_size(t_text::get_size());
        return;
    }

    template <typename t_text>
    auto text_input_constraint_impl_t<t_text>::text_input_key_callback(int key, int action, int mods) -> void
    {
        if (action != GLFW_PRESS && action != GLFW_REPEAT)
            return;
        std::unique_lock lock{this->m_mutex};
        if (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_DELETE)
            text_input_cha_callback(U'\b');
        else if (mods & GLFW_MOD_CONTROL)
        {
            if (key == GLFW_KEY_A)
                m_select_all = true;
            else if (key == GLFW_KEY_C)
                glfwSetClipboardString(m_engine->window_get(), t_text::get_text().c_str());
            else if (key == GLFW_KEY_V)
            {
                const auto* clipboard{glfwGetClipboardString(m_engine->window_get())};
                if (!clipboard)
                    return;
                std::string text{m_select_all ? std::string{} : t_text::get_text()};
                m_select_all = false;
                const std::string_view input{clipboard};
                for (std::size_t index{}; index < input.size();)
                {
                    const auto first{static_cast<unsigned char>(input[index++])};
                    char32_t code{first};
                    unsigned continuation{};
                    if (first >= 0xc2 && first <= 0xdf)
                    {
                        code = first & 0x1f;
                        continuation = 1;
                    }
                    else if (first >= 0xe0 && first <= 0xef)
                    {
                        code = first & 0x0f;
                        continuation = 2;
                    }
                    else if (first >= 0xf0 && first <= 0xf4)
                    {
                        code = first & 0x07;
                        continuation = 3;
                    }
                    else if (first >= 0x80)
                        continue;
                    if (index + continuation > input.size())
                        break;
                    bool valid{true};
                    for (unsigned part{}; part < continuation; ++part)
                    {
                        const auto byte{static_cast<unsigned char>(input[index++])};
                        if ((byte & 0xc0) != 0x80)
                        {
                            valid = false;
                            break;
                        }
                        code = (code << 6) | (byte & 0x3f);
                    }
                    if (!valid)
                        continue;
                    if (m_constraint)
                        m_constraint(&text, code);
                    else
                        constraint_default(&text, code);
                }
                t_text::set_text(std::move(text));
                m_click_area.set_size(t_text::get_size());
            }
        }
    }

    template <typename t_text>
    auto text_input_constraint_impl_t<t_text>::text_input_click_area_callback() -> void
    {
        std::unique_lock lock{this->m_mutex};
        if (m_focus)
        {
            m_focus->exclusive(std::bind(&text_input_constraint_impl_t::text_input_on_focus_callback, this));
        }
        t_text::set_frame_color(std_yellow);
        if (!m_on_focus)
        {
            set_callback();
        }
        m_on_focus = true;
        return;
    }

    template <typename t_text>
    auto text_input_constraint_impl_t<t_text>::text_input_on_focus_callback() -> void
    {
        std::unique_lock lock{this->m_mutex};
        m_on_focus = false;
        t_text::set_frame_color(std_white);
        clear_callback();
        return;
    }

    template <typename t_text> auto text_input_constraint_impl_t<t_text>::set_callback() -> void
    {
        std::unique_lock lock{this->m_mutex};
        if (m_callback)
            return;
        m_key_callback = m_engine->callback_key_add([this](int key, int action, int mods)
                                                    { text_input_key_callback(key, action, mods); });
        m_callback = m_engine->callback_cha_add(
            std::bind(&text_input_constraint_impl_t::text_input_cha_callback, this, std::placeholders::_1));
        return;
    }

    template <typename t_text> auto text_input_constraint_impl_t<t_text>::clear_callback() -> void
    {
        if (m_callback)
            m_engine->callback_cha_remove(m_callback);
        m_callback = nullptr;
        if (m_key_callback)
            m_engine->callback_key_remove(m_key_callback);
        m_key_callback = nullptr;
        return;
    }

    template <typename t_text>
    auto text_input_constraint_impl_t<t_text>::constraint_default(std::string* text, char32_t in) -> void
    {
        if (in == U'\b')
        {
            if (!text->empty())
            {
                std::size_t begin{text->size() - 1};
                while (begin && (static_cast<unsigned char>((*text)[begin]) & 0xc0) == 0x80)
                    --begin;
                text->resize(begin);
            }
        }
        else if (in >= 0x20 && in <= 0x10ffff && !(in >= 0xd800 && in <= 0xdfff))
        {
            if (in <= 0x7f)
                text->push_back(static_cast<char>(in));
            else
            {
                if (in <= 0x7ff)
                    text->push_back(static_cast<char>(0xc0 | (in >> 6)));
                else
                {
                    if (in <= 0xffff)
                        text->push_back(static_cast<char>(0xe0 | (in >> 12)));
                    else
                    {
                        text->push_back(static_cast<char>(0xf0 | (in >> 18)));
                        text->push_back(static_cast<char>(0x80 | ((in >> 12) & 0x3f)));
                    }
                    text->push_back(static_cast<char>(0x80 | ((in >> 6) & 0x3f)));
                }
                text->push_back(static_cast<char>(0x80 | (in & 0x3f)));
            }
        }
        return;
    }

    template <typename t_text>
    auto text_input_constraint_impl_t<t_text>::only_num(std::string* text, char32_t in) -> void
    {
        if ((in >= L'0' && in <= L'9') || (in >= L'A' && in <= L'F') || in == L'\b')
        {
            constraint_default(text, in);
        }
        return;
    }

    template <typename t_text>
    auto text_input_constraint_impl_t<t_text>::limit_len(std::string* text, char32_t in, std::size_t len)
        -> void
    {
        if (text->size() < len || in == L'\b')
        {
            constraint_default(text, in);
        }
        return;
    }

    template <typename t_text>
    auto text_input_constraint_impl_t<t_text>::only_num_limit_len(std::string* text, char32_t in,
                                                                  std::size_t len) -> void
    {
        if ((text->size() < len && ((in >= L'0' && in <= L'9') || (in >= L'A' && in <= L'F'))) || in == L'\b')
        {
            constraint_default(text, in);
        }
        return;
    }

    template <typename t_text>
    template <typename... t_arg>
    text_input_constraint_impl_t<t_text>::text_input_constraint_impl_t(
        engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range, pos_2D pos,
        std::string text, std::function<void(std::string*, char32_t)> constraint, focus_t* focus,
        t_arg&&... arg)
        : t_text{engine, depth_tracker, depth_range, pos, text, std::forward<t_arg>(arg)...},
          m_engine{engine}, m_focus{focus},
          m_click_area{engine,
                       depth_tracker,
                       depth_range,
                       pos,
                       t_text::get_size(),
                       []() {},
                       [](pos_2D) {},
                       [](pos_2D) {},
                       [](pos_2D) {},
                       std::bind(&text_input_constraint_impl_t::text_input_click_area_callback, this)},
          m_constraint{constraint}
    {
    }

    template <typename t_text> auto text_input_constraint_impl_t<t_text>::set_pos(pos_2D pos) -> void
    {
        std::unique_lock lock{this->m_mutex};
        t_text::set_pos(pos);
        m_click_area.set_pos(pos);
        return;
    }

    template <typename t_text>
    auto text_input_constraint_impl_t<t_text>::log(std::uint32_t tab_count) const -> void
    {
        if constexpr (!gui_object_names_enabled)
            return;
        log_file(tab(tab_count) + "{\n");
        log_file(tab(tab_count + 1) + "type: text_input_constraint_impl_t<>\n");
        t_text::log(tab_count + 1);
        log_file(tab(tab_count + 1) + "click_area:\n");
        m_click_area.log(tab_count + 1);
        log_file(tab(tab_count) + "}\n");
        return;
    }

    template <typename t_text> auto text_input_constraint_impl_t<t_text>::show_impl(bool base) -> void
    {
        if (!this->show_begin(base))
        {
            return;
        }
        std::unique_lock lock{this->m_mutex};
        t_text::show_impl(false);
        m_click_area.show();
        if (m_on_focus)
        {
            set_callback();
        }
        this->show_end(base);
        return;
    }

    template <typename t_text> auto text_input_constraint_impl_t<t_text>::hide_impl(bool base) -> void
    {
        if (!this->hide_begin(base))
        {
            return;
        }
        std::unique_lock lock{this->m_mutex};
        t_text::hide_impl(false);
        m_click_area.hide();
        if (m_on_focus)
        {
            if (m_focus)
                m_focus->clear();
            else
            {
                clear_callback();
                m_on_focus = false;
            }
        }
        this->hide_end(base);
        return;
    }

    using text_line_std_input_constraint_t = text_input_constraint_impl_t<text_line_std_t>;
    using text_line_std_input_constraint_flex_t = text_input_constraint_impl_t<text_line_flex_std_t>;
    // using text_block_scroll_input_constraint_t = text_input_constraint_impl_t<text_block_scroll_t>;
    // using text_block_scroll_std_input_constraint_t = text_input_constraint_impl_t<text_block_scroll_std_t>;

    template <typename t_text> class text_input_impl_t : public text_input_constraint_impl_t<t_text>
    {
      public:
        text_input_impl_t() = default;
        template <typename... t_arg>
        text_input_impl_t(engine_t* engine, depth_tracker_t* depth_tracker, depth_range_t depth_range,
                          pos_2D pos, std::string text, focus_t* focus, t_arg&&... arg);
    };

    template <typename t_text>
    template <typename... t_arg>
    text_input_impl_t<t_text>::text_input_impl_t(engine_t* engine, depth_tracker_t* depth_tracker,
                                                 depth_range_t depth_range, pos_2D pos, std::string text,
                                                 focus_t* focus, t_arg&&... arg)
        : text_input_constraint_impl_t<t_text>{
              engine, depth_tracker, depth_range, pos, text, {}, focus, std::forward<t_arg>(arg)...}
    {
    }

    using text_line_std_input_t = text_input_impl_t<text_line_std_t>;
    using text_line_flex_std_input_t = text_input_impl_t<text_line_flex_std_t>;
    // using text_block_scroll_input_t = text_input_impl_t<text_block_scroll_t>;
    // using text_block_scroll_std_input_t = text_input_impl_t<text_block_scroll_std_t>;
} // namespace lgo
