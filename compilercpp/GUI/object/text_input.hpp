#ifndef COMPILERCPP_GUI_OBJECT_TEXT_INPUT
#define COMPILERCPP_GUI_OBJECT_TEXT_INPUT

#include <functional>

#include "text_scroll.hpp"
#include "click_area.hpp"
#include "std.hpp"
#include "focus.hpp"

#include "../../lib/stu.hpp"
#include "../primitive/engine_decl.hpp"

//engine_t* engine,
//depth_tracker_t* depth_tracker,
//depth_range_t depth_range,
//pos_2D pos,
//std::string text,
//std::function<void(std::wstring*, wchar_t)> constraint,
//focus_t* focus,
//t_arg&&... arg
template<typename t_text>
class text_input_constraint_impl_t: public t_text {
private:
    engine_t* m_engine{};
    focus_t* m_focus{};
    std::atomic_bool m_on_focus{ false };
    click_area_t m_click_area{};
    std::function<void(wchar_t)>* m_callback{};
    std::function<void(std::string*, wchar_t)> m_constraint{};

    auto text_input_charw_callback(wchar_t in) -> void;
    auto text_input_click_area_callback() -> void;
    auto text_input_on_focus_callback() -> void;

    auto set_callback() -> void;
    auto clear_callback() -> void;
public:
    static auto constraint_default(std::string* text, wchar_t in) -> void;
    static auto only_num(std::string* text, wchar_t in) -> void;
    static auto limit_len(std::string* text, wchar_t in, std::size_t len) -> void;
    static auto only_num_limit_len(std::string* text, wchar_t in, std::size_t len) -> void;

    text_input_constraint_impl_t() = default;
    template<typename... t_arg>
    text_input_constraint_impl_t(
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        std::function<void(std::string*, wchar_t)> constraint,
        focus_t* focus,
        t_arg&&... arg
    );
    
    auto set_pos(pos_2D pos) -> void override;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;
};

template<typename t_text>
auto text_input_constraint_impl_t<t_text>::text_input_charw_callback(wchar_t in) -> void {
    std::unique_lock lock{ this->m_mutex };
    std::string text{ t_text::get_text() };
    if (m_constraint) {
        m_constraint(&text, in);
    }
    else {
        constraint_default(&text, in);
    }
    t_text::set_text(text);
    m_click_area.set_size(t_text::get_size());
    return;
}

template<typename t_text>
auto text_input_constraint_impl_t<t_text>::text_input_click_area_callback() -> void {
    std::unique_lock lock{ this->m_mutex };
    if (m_focus) {
        m_focus->exclusive(std::bind(&text_input_constraint_impl_t::text_input_on_focus_callback, this));
    }
    t_text::set_frame_color(std_yellow);
    if (!m_on_focus) {
        set_callback();
    }
    m_on_focus = true;
    return;
}

template<typename t_text>
auto text_input_constraint_impl_t<t_text>::text_input_on_focus_callback() -> void {
    std::unique_lock lock{ this->m_mutex };
    m_on_focus = false;
    t_text::set_frame_color(std_white);
    clear_callback();
    return;
}

template<typename t_text>
auto text_input_constraint_impl_t<t_text>::set_callback() -> void {
    std::unique_lock lock{ this->m_mutex };
    m_callback = m_engine->add_charw(std::bind(&text_input_constraint_impl_t::text_input_charw_callback, this, std::placeholders::_1));
    return;
}

template<typename t_text>
auto text_input_constraint_impl_t<t_text>::clear_callback() -> void {
    m_engine->remove_charw(m_callback);
    return;
}

template<typename t_text>
auto text_input_constraint_impl_t<t_text>::constraint_default(std::string* text, wchar_t in) -> void {
    if (in == L'\b') {
        auto wide_text{ to_wstring(*text) };
        if (!wide_text.empty()) {
            auto last{ wide_text.back() };
            wide_text.pop_back();
            if (last >= 0xDC00 && last <= 0xDFFF && !wide_text.empty()
                && wide_text.back() >= 0xD800 && wide_text.back() <= 0xDBFF) {
                wide_text.pop_back();
            }
            *text = to_string(wide_text);
        }
    }
    else {
        *text += to_string(std::wstring{ in });
    }
    return;
}

template<typename t_text>
auto text_input_constraint_impl_t<t_text>::only_num(std::string* text, wchar_t in) -> void {
    if ((in >= L'0' && in <= L'9') || (in >= L'A' && in <= L'F') || in == L'\b') {
        constraint_default(text, in);
    }
    return;
}

template<typename t_text>
auto text_input_constraint_impl_t<t_text>::limit_len(std::string* text, wchar_t in, std::size_t len) -> void {
    if (text->size() < len || in == L'\b') {
        constraint_default(text, in);
    }
    return;
}

template<typename t_text>
auto text_input_constraint_impl_t<t_text>::only_num_limit_len(std::string* text, wchar_t in, std::size_t len) -> void {
    if ((text->size() < len && ((in >= L'0' && in <= L'9') || (in >= L'A' && in <= L'F'))) || in == L'\b') {
        constraint_default(text, in);
    }
    return;
}

template<typename t_text>
template<typename... t_arg>
text_input_constraint_impl_t<t_text>::text_input_constraint_impl_t(
    engine_t* engine,
    depth_tracker_t* depth_tracker,
    depth_range_t depth_range,
    pos_2D pos,
    std::string text,
    std::function<void(std::string*, wchar_t)> constraint,
    focus_t* focus,
    t_arg&&... arg
):
    t_text{
        engine,
        depth_tracker,
        depth_range,
        pos,
        text,
        std::forward<t_arg>(arg)...
    },
    m_engine{ engine },
    m_focus{ focus },
    m_click_area{
        engine,
        depth_tracker,
        depth_range_t{ depth_range.near - 1.0f, depth_range.far - 1.0f },
        pos,
        t_text::get_size(),
        [] (pos_2D) {},
        [] () {},
        [] (pos_2D) {},
        [] (pos_2D) {},
        std::bind(&text_input_constraint_impl_t::text_input_click_area_callback, this)
    },
    m_constraint{ constraint }
{}

template<typename t_text>
auto text_input_constraint_impl_t<t_text>::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ this->m_mutex };
    t_text::set_pos(pos);
    m_click_area.set_pos(pos);
    return;
}

template<typename t_text>
auto text_input_constraint_impl_t<t_text>::show_impl(bool base) -> void {
    if (!this->show_begin(base)) { return; }
    std::unique_lock lock{ this->m_mutex };
    t_text::show_impl(false);
    m_click_area.show();
    if (m_on_focus) {
        set_callback();
    }
    this->show_end(base);
    return;
}

template<typename t_text>
auto text_input_constraint_impl_t<t_text>::hide_impl(bool base) -> void {
    if (!this->hide_begin(base)) { return; }
    std::unique_lock lock{ this->m_mutex };
    t_text::hide_impl(false);
    m_click_area.hide();
    if (m_on_focus) {
        clear_callback();
    }
    this->hide_end(base);
    return;
}

using text_line_std_input_constraint_t = text_input_constraint_impl_t<text_line_std_t>;
using text_line_std_input_constraint_flex_t = text_input_constraint_impl_t<text_line_flex_std_t>;
//using text_block_scroll_input_constraint_t = text_input_constraint_impl_t<text_block_scroll_t>;
//using text_block_scroll_std_input_constraint_t = text_input_constraint_impl_t<text_block_scroll_std_t>;

//engine_t* engine,
//depth_tracker_t* depth_tracker,
//depth_range_t depth_range,
//pos_2D pos,
//std::string text,
//focus_t* focus,
//t_arg&&... arg
//v
//engine,
//depth_tracker,
//depth_range,
//pos,
//text,
//arg...
template<typename t_text>
class text_input_impl_t: public text_input_constraint_impl_t<t_text> {
public:
    text_input_impl_t() = default;
    template<typename... t_arg>
    text_input_impl_t(
        engine_t* engine,
        depth_tracker_t* depth_tracker,
        depth_range_t depth_range,
        pos_2D pos,
        std::string text,
        focus_t* focus,
        t_arg&&... arg
    );
};

template<typename t_text>
template<typename... t_arg>
text_input_impl_t<t_text>::text_input_impl_t(
    engine_t* engine,
    depth_tracker_t* depth_tracker,
    depth_range_t depth_range,
    pos_2D pos,
    std::string text,
    focus_t* focus,
    t_arg&&... arg
):
    text_input_constraint_impl_t<t_text>{
        engine,
        depth_tracker,
        depth_range,
        pos,
        text,
        {},
        focus,
        std::forward<t_arg>(arg)...
    }
{}

using text_line_std_input_t = text_input_impl_t<text_line_std_t>;
using text_line_flex_std_input_t = text_input_impl_t<text_line_flex_std_t>;
//using text_block_scroll_input_t = text_input_impl_t<text_block_scroll_t>;
//using text_block_scroll_std_input_t = text_input_impl_t<text_block_scroll_std_t>;

#endif
