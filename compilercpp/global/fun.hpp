#ifndef COMPILERCPP_GLOBAL_FUN
#define COMPILERCPP_GLOBAL_FUN

#include <string>
#include <vector>
#include <iostream>

#include "pv.hpp"
#include "../GUI/.hpp"
#include "../stmt/.hpp"
#include "../lib/.hpp"

class fun_GUI_t;

class fun: public global_pv {
private:
    std::wstring m_name{};
    std::vector<stmt> m_stmt{};
    fun_GUI_t* m_GUI{};

    friend class fun_GUI_t;
public:
    fun(std::istream& src) {
        m_name = get_stringw(src);
        std::size_t stmt_count{ get_integer<std::uint64_t>(src) };
        for (std::size_t i{ 0 }; i < stmt_count; ++i) {
            m_stmt.push_back(stmt{ src });
        }
        return;
    }

    virtual ~fun() = default;

    auto name() const -> const std::wstring& {
        return m_name;
    }

    auto save(std::ostream& dest) -> void {
        pad_stringw(dest, m_name);
        pad_integer<std::uint64_t>(dest, m_stmt.size());
        for (std::size_t i{ 0 }; i < m_stmt.size(); ++i) {
            m_stmt[i].save(dest);
        }
        return;
    }

    auto compile(code_stream& dest) const -> void {
        for (std::uint64_t i{ 0 }; i < m_stmt.size(); ++i) {
            m_stmt[i].compile(dest);
        }
        return;
    }
};

class fun_GUI_t: public std_divy_flex_border_t {
private:
    fun* m_data{};
public:
    static color_t color;

    fun_GUI_t() = default;
    fun_GUI_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, focus_t* focus, size_1D size_x, depth_range_t depth_range, fun* data);

    auto get_margin() const -> size_1D override;
};

color_t fun_GUI_t::color{ std_red };

fun_GUI_t::fun_GUI_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, focus_t* focus, size_1D size_x, depth_range_t depth_range, fun* data)
: std_divy_flex_border_t{ engine, depth_tracker, pos, size_x, color, alignment_2D{ alignment_x::left, alignment_y::top }
, depth_range_t{ (depth_range.near + depth_range.far) / 2, depth_range.far } }, m_data{ data } {
    m_data->m_GUI = this;
    auto header{ this->add_object_shared<divx_soft_t>(div_create_from_member{}, std_forward, std_margin
    , std_divy_flex_border_t::space_x(std_margin), alignment_2D{ alignment_x::left, alignment_y::center }) };
    header->add_object<std_text_line_t>(L"函數", std_sideways, color);
    header->add_object<std_liney_t>(std_forward, color, depth_range.near);
    header->add_object<std_text_line_t>(m_data->m_name, std_divy_flex_border_t::space_x(std_margin), std_white);
    this->add_object<std_linex_t>(std_divy_flex_border_t::space_x(std_margin), color, depth_range.near);
    for (std::size_t i{ 0 }; i < m_data->m_stmt.size(); ++i) {
        this->add_object<stmt_GUI_t>(focus, this->space_x(std_margin), depth_range_t{ depth_range.near, (depth_range.near + depth_range.far) / 2 }, &m_data->m_stmt[i]);
    }
    return;
}

auto fun_GUI_t::get_margin() const -> size_1D {
    return margin;
}

#endif