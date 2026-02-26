#ifndef COMPILERCPP_STMT_MAIN
#define COMPILERCPP_STMT_MAIN

#include <cstdint>
#include <iostream>
#include <vector>

#include "../lib/.hpp"
#include "../instr/.hpp"
#include "../data/.hpp"

#include "pv.hpp"

class stmt {
public:
    using type_t = std::uint8_t;

    enum: type_t {
        expr,
        instr,
        data_bi,
        ret
    };
private:
    type_t m_type{};
    stmt_pv* m_data{};

    friend class stmt_GUI_t;
public:
    stmt(std::istream& src) {
        m_type = get_integer<type_t>(src);
        switch (m_type) {
        case instr:
            m_data = new ::instr{ src };
            break;
        case data_bi:
            m_data = new ::data_bi{ src };
            break;
        default:
            throw error_t{ "敘述種類錯誤" };
        }
        return;
    }

    auto get_type() -> type_t {
        return m_type;
    }

    auto save(std::ostream& dest) -> void {
        pad_integer<type_t>(dest, m_type);
        m_data->save(dest);
        return;
    }

    auto compile(code_stream& dest) const -> void {
        switch (m_type) {
        case instr:
            dynamic_cast<::instr*>(m_data)->assemble(dest);
            break;
        case data_bi:
            dynamic_cast<::data_bi*>(m_data)->compile(dest);
            break;
        default:
            throw error_t{ "敘述種類錯誤" };
        }
        return;
    }
};

class stmt_GUI_t: public GUI_object_t {
private:
    std_divx_border_t m_div{};
    std::weak_ptr<GUI_object_t> m_GUI{};

    auto stmt_GUI_menu_callback() -> void;
public:
    stmt_GUI_t() = default;
    stmt_GUI_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, focus_t* focus, size_1D size_x, depth_range_t depth_range, stmt* data);

    auto get_pos() const -> pos_2D;
    auto set_pos(pos_2D pos) -> void;
    auto get_size() const -> size_2D;
    auto get_margin() const -> size_1D;

    auto show() -> void override;
    auto hide() -> void override;
};

auto stmt_GUI_t::stmt_GUI_menu_callback() -> void {
    return;
}

stmt_GUI_t::stmt_GUI_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, focus_t* focus, size_1D size_x, depth_range_t depth_range, stmt* data)
: m_div{ engine, depth_tracker, pos, div_create_from_member{}, std_forward, std_margin, size_x, instr_GUI_t::color, alignment_2D{ alignment_x::left, alignment_y::center }
, depth_range_t{ (depth_range.near + depth_range.far) / 2, depth_range.far } } {
    m_div.add_object<text_click_area_t<std_text_line_t>>(L"指令", std_sideways, depth_range.near, instr_GUI_t::color, std::bind(&stmt_GUI_t::stmt_GUI_menu_callback, this));
    m_div.add_object<std_liney_t>(std_forward, instr_GUI_t::color, depth_range.near);
    switch (data->m_type) {
    case stmt::expr: {
        break;
    }
    case stmt::instr: {
        m_div.add_object<instr_GUI_t>(focus, size_x, depth_range_t{ depth_range.near, (depth_range.near + depth_range.far) / 2 }, dynamic_cast<instr*>(data->m_data));
        break;
    }
    case stmt::data_bi: {
        m_div.add_object<instr_GUI_t>(focus, size_x, depth_range_t{ depth_range.near, (depth_range.near + depth_range.far) / 2 }, dynamic_cast<instr*>(data->m_data));
        break;
    }
    case stmt::ret: {
        break;
    }
    }
}

auto stmt_GUI_t::get_pos() const -> pos_2D {
    return m_div.get_pos();
}

auto stmt_GUI_t::set_pos(pos_2D pos) -> void {
    return m_div.set_pos(pos);
}

auto stmt_GUI_t::get_size() const -> size_2D {
    return m_div.get_size();
}

auto stmt_GUI_t::get_margin() const -> size_1D {
    return m_div.get_margin();
}

auto stmt_GUI_t::show() -> void {
    m_div.show();
    GUI_object_t::show();
    return;
}

auto stmt_GUI_t::hide() -> void {
    GUI_object_t::hide();
    m_div.hide();
    return;
}


#endif