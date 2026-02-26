#ifndef COMPILERCPP_GUI_OBJECT_MENU
#define COMPILERCPP_GUI_OBJECT_MENU

#include "object_def.hpp"
#include "click_area.hpp"
#include "rect.hpp"
#include "text.hpp"

#include "../primitive/engine_def.hpp"

/*class menu_t: public GUI_object_t {
private:
    struct option_t {
        text_block_t m_text{};
        click_area_t m_click_area{};
    };
    engine_t* m_engine{};
    depth_tracker_t* m_depth_tracker{};
    std::vector<option_t> m_option{};
    rect_t m_rect{};
    pos_2D m_pos{};
    float m_depth{};
    size_2D m_option_size{};

    auto menu_mouse_move_callback(std::size_t index, pos_2D) -> void;
    auto menu_mouse_leave_callback(std::size_t index) -> void;
    auto menu_mouse_left_click_callback(std::size_t index, pos_2D) -> void;
    auto menu_mouse_left_release_callback(std::size_t index, pos_2D) -> void;
public:
    menu_t() = default;
    menu_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D option_size, float depth);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;

    auto show_impl(bool base) -> void override;
    auto hide_impl(bool base) -> void override;

    auto add_option(std::wstring text, std::function<void(pos_2D)> callback) -> void;
};

auto menu_t::menu_mouse_move_callback(std::size_t index, pos_2D) -> void {
    std::unique_lock lock{ m_mutex };
    m_rect.set_pos(m_option[index].m_click_area.get_pos());
    m_rect.set_color(std_dark_gray);
    m_rect.show();
    return;
}

auto menu_t::menu_mouse_leave_callback(std::size_t index) -> void {
    std::unique_lock lock{ m_mutex };
    if (m_option[index].m_click_area.get_pos() == m_rect.get_pos()) {
        m_rect.hide();
    }
    m_option[index].m_text.set_color(std_white);
    return;
}

auto menu_t::menu_mouse_left_click_callback(std::size_t index, pos_2D) -> void {
    std::unique_lock lock{ m_mutex };
    m_rect.set_color(std_white);
    m_option[index].m_text.set_color(std_black);
    return;
}

auto menu_t::menu_mouse_left_release_callback(std::size_t index, pos_2D) -> void {
    std::unique_lock lock{ m_mutex };
    m_rect.set_color(std_dark_gray);
    m_rect.set_pos(m_option[index].m_click_area.get_pos());
    m_rect.show();
    m_option[index].m_text.set_color(std_white);
    return;
}

menu_t::menu_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, size_2D option_size, float depth)
: m_engine{ engine }, m_depth_tracker{ depth_tracker }
, m_rect{ engine, depth_tracker, pos, option_size, depth, std_dark_gray }, m_pos{ pos }, m_depth{ depth }, m_option_size{ option_size } {}

auto menu_t::get_pos() const -> pos_2D {
    std::unique_lock lock{ m_mutex };
    return m_pos;
}

auto menu_t::set_pos(pos_2D pos) -> void {
    std::unique_lock lock{ m_mutex };
    size_2D offset{ pos - m_pos };
    m_pos = pos;
    for (std::size_t i{ 0 }; i < m_option.size(); ++i) {
        m_option[i].m_text.set_pos(m_option[i].m_text.get_pos() + offset);
        m_option[i].m_click_area.set_pos(m_option[i].m_click_area.get_pos() + offset);
    }
    return;
}

auto menu_t::get_size() const -> size_2D {
    std::unique_lock lock{ m_mutex };
    return size_2D{ m_option_size.x, m_option_size.y * static_cast<std::int64_t>(m_option.size()) };
}

auto menu_t::get_margin() const -> size_1D {
    return size_1D{ 0 };
}

auto menu_t::show_impl(bool base) -> void {
    if (!show_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    for (std::size_t i{ 0 }; i < m_option.size(); ++i) {
        m_option[i].m_text.show();
        m_option[i].m_click_area.show();
    }
    show_end(base);
    return;
}

auto menu_t::hide_impl(bool base) -> void {
    if (!hide_begin(base)) { return; }
    std::unique_lock lock{ m_mutex };
    for (std::size_t i{ 0 }; i < m_option.size(); ++i) {
        m_option[i].m_text.hide();
        m_option[i].m_click_area.hide();
    }
    m_rect.hide();
    hide_end(base);
    return;
}

auto menu_t::add_option(std::wstring text, std::function<void(pos_2D)> callback) -> void {
    std::unique_lock lock{ m_mutex };
    pos_2D option_pos{ m_pos.x, m_pos.y + m_option_size.y * static_cast<std::int64_t>(m_option.size()) };
    m_option.push_back(option_t{ text_block_t{ m_engine, m_depth_tracker, option_pos, m_option_size, text, option_pos, m_option_size, std_font_size, std_white
    , alignment_2D{ alignment_x::center, alignment_y::center } }, click_area_t{ m_engine, m_depth_tracker, option_pos, m_option_size, m_depth
    , std::bind(&menu_t::menu_mouse_move_callback, this, m_option.size(), std::placeholders::_1), std::bind(&menu_t::menu_mouse_leave_callback, this, m_option.size())
    , std::bind(&menu_t::menu_mouse_left_click_callback, this, m_option.size(), std::placeholders::_1)
    , std::bind(&menu_t::menu_mouse_left_release_callback, this, m_option.size(), std::placeholders::_1)
    , std::bind(callback, pos_2D{ option_pos.x + m_option_size.x, option_pos.y }) } });
    return;
}*/

#endif