#ifndef COMPILERCPP_DATA_BI
#define COMPILERCPP_DATA_BI

#include <iostream>
#include <cstdint>
#include <string>

#include "../stmt/pv.hpp"
#include "../data_type/.hpp"
#include "../lib/.hpp"
#include "../code/stream.hpp"
#include "../GUI/.hpp"

class data_bi_pv;
class data_bi_GUI_t;
template<typename t_data>
class data_bi_impl;
class data_bi;

class data_bi_pv {
public:
    virtual ~data_bi_pv() = default;
    virtual auto save(std::ostream& dest) -> void = 0;
    virtual auto compile(code_stream& dest) -> void = 0;
    virtual auto val_to_strw() -> std::wstring = 0;
    virtual auto val_set(std::wstring val) -> void = 0;
};

template<typename t_data>
class data_bi_impl: public data_bi_pv {
private:
    t_data m_data{};
public:
    data_bi_impl(std::istream& src);

    auto save(std::ostream& dest) -> void;
    auto compile(code_stream& dest) -> void;
    auto val_to_strw() -> std::wstring;
    auto val_set(std::wstring val) -> void;
};

class data_bi: public stmt_pv {
private:
    size_t m_size{};
    data_bi_pv* m_data{};
    data_bi_GUI_t* m_GUI{};
    std::wstring m_name{};

    friend class data_bi_GUI_t;
public:
    using size_t = std::uint8_t;

    enum size: size_t {
        size_begin,
        byte = size_begin,
        word,
        dword,
        qword,
        oword,
        size_end
    };

    static auto size_to_uint8(size_t in) -> std::uint8_t;
    static auto size_to_stringw(size_t in) -> std::wstring;

    template<typename t_data>
    static auto type_to_size() -> size_t;

    data_bi(std::istream& src);

    auto save(std::ostream& dest) -> void;
    auto compile(code_stream& dest) -> void;
    auto val_to_strw() -> std::wstring;
    auto name_set(std::wstring name) -> void;
    auto size_set(size_t size) -> void;
    auto val_set(std::wstring val) -> void;
};

class data_bi_GUI_t: public GUI_object_t {
private:
    divx_soft_t m_div{};
    data_bi* m_data{};
    engine_t* m_engine{};
    depth_tracker_t* m_depth_tracker{};
    depth_range_t m_depth_range{};
    std::vector<focus_t*> m_focus_list{};
    std::uint64_t m_plus_id{};

    auto data_bi_GUI_name_change_callback(pos_2D) -> void;
    auto data_bi_GUI_name_set_callback(std::shared_ptr<std_text_line_input_flex_t> name) -> void;
    auto data_bi_GUI_size_change_callback(pos_2D) -> void;
    auto data_bi_GUI_size_set_callback(data_bi::size_t size, pos_2D) -> void;
    auto data_bi_GUI_val_change_callback(pos_2D) -> void;
    auto data_bi_GUI_val_set_callback(std::shared_ptr<std_text_line_input_constraint_flex_t> val) -> void;
    auto data_bi_GUI_focus_callback(GUI_object_t* object, std::size_t next_index) -> void;

    friend class operand_GUI_t;
public:
    static color_t color;

    data_bi_GUI_t() = default;
    data_bi_GUI_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, focus_t* focus, size_1D size_x, depth_range_t depth_range, data_bi* data);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;

    auto show() -> void override;
    auto hide() -> void override;
    auto update() -> void;
};

template<typename t_data>
data_bi_impl<t_data>::data_bi_impl(std::istream& src) {
    m_data = get_integer<t_data>(src);
    return;
}

template<typename t_data>
auto data_bi_impl<t_data>::save(std::ostream& dest) -> void {
    pad_integer(dest, m_data);
    return;
}

template<typename t_data>
auto data_bi_impl<t_data>::compile(code_stream& dest) -> void {
    dest.pad_int<t_data>(m_data);
    return;
}

template<typename t_data>
auto data_bi_impl<t_data>::val_to_strw() -> std::wstring {
    if constexpr (std::is_same_v<t_data, uint128_t>) {
        return to_wstring(m_data.m_significant_most, data_bi::type_to_size<t_data>()) + to_wstring(m_data.m_significant_least, data_bi::type_to_size<t_data>());
    }
    else {
        return to_wstring(m_data, data_bi::type_to_size<t_data>());
    }
}

template<typename t_data>
auto data_bi_impl<t_data>::val_set(std::wstring val) -> void {
    if constexpr (std::is_same_v<t_data, uint128_t>) {
        if (val.size() > 16) {
            m_data.m_significant_most = std::stoull(val.substr(0, val.size() - 16));
            m_data.m_significant_least = std::stoull(val.substr(val.size() - 16, 16));
        }
        else {
            m_data.m_significant_most = 0;
            m_data.m_significant_least = std::stoull(val);
        }
    }
    else {
        m_data = static_cast<t_data>(std::stoull(val));
    }
    return;
}

auto data_bi::size_to_uint8(size_t in) -> std::uint8_t {
    switch (in) {
    case byte:
        return 1;
    case word:
        return 2;
    case dword:
        return 4;
    case qword:
        return 8;
    case oword:
        return 16;
    default:
        throw internal_error_t{ "size_to_size" };
    }
}

auto data_bi::size_to_stringw(size_t in) -> std::wstring {
    switch (in) {
    case byte:
        return L"byte";
    case word:
        return L"word";
    case dword:
        return L"dword";
    case qword:
        return L"qword";
    case oword:
        return L"oword";
    default:
        throw internal_error_t{ "size_to_size" };
    }
}

template<typename t_data>
auto data_bi::type_to_size() -> size_t {
    if constexpr (std::is_same_v<t_data, std::uint8_t>) {
        return byte;
    } else if constexpr (std::is_same_v<t_data, std::uint16_t>) {
        return word;
    } else if constexpr (std::is_same_v<t_data, std::uint32_t>) {
        return dword;
    } else if constexpr (std::is_same_v<t_data, std::uint64_t>) {
        return qword;
    } else if constexpr (std::is_same_v<t_data, uint128_t>) {
        return oword;
    } else {
        throw internal_error_t{ "data_bi size" };
    }
}

data_bi::data_bi(std::istream& src) {
    m_size = get_integer<size_t>(src);
    switch (m_size) {
    case byte:
        m_data = new data_bi_impl<std::uint8_t>{ src };
        break;
    case word:
        m_data = new data_bi_impl<std::uint16_t>{ src };
        break;
    case dword:
        m_data = new data_bi_impl<std::uint32_t>{ src };
        break;
    case qword:
        m_data = new data_bi_impl<std::uint64_t>{ src };
        break;
    case oword:
        m_data = new data_bi_impl<uint128_t>{ src };
        break;
    default:
        throw error_t{ "data_bi size" };
    }
    return;
}

auto data_bi::save(std::ostream& dest) -> void {
    pad_integer(dest, size_to_uint8(m_size));
    m_data->save(dest);
    return;
}

auto data_bi::compile(code_stream& dest) -> void {
    m_data->compile(dest);
    return;
}

auto data_bi::val_to_strw() -> std::wstring {
    return m_data->val_to_strw();
}

auto data_bi::name_set(std::wstring name) -> void {
    m_name = name;
    m_GUI->update();
    return;
}

auto data_bi::size_set(size_t size) -> void {
    m_data = nullptr;
    m_size = size;
    m_GUI->update();
    return;
}

auto data_bi::val_set(std::wstring val) -> void {
    m_data->val_set(val);
    m_GUI->update();
    return;
}

auto data_bi_GUI_t::data_bi_GUI_name_change_callback(pos_2D) -> void {
    auto name_focus{ new focus_t{} };
    auto name_div{ new divx_flex_soft_t{ m_engine, m_depth_tracker, pos_2D{ get_pos().x, get_pos().y + get_size().y }, div_create_from_member{}
    , std_forward, std_margin, alignment_2D{ alignment_x::left, alignment_y::top } } };
    auto name_input{ name_div->add_object_shared<std_text_line_input_flex_t>(m_depth_range.near, L"", name_focus, std_sideways, std_white) };
    name_div->add_object<button_classic_std_t>(std::bind(&data_bi_GUI_t::data_bi_GUI_name_set_callback, this, name_input), L"確定", m_depth_range);
    m_focus_list[0]->exclusive(std::bind(&data_bi_GUI_t::data_bi_GUI_focus_callback, this, name_div, 1));
    m_focus_list.push_back(name_focus);
    return;
}

auto data_bi_GUI_t::data_bi_GUI_name_set_callback(std::shared_ptr<std_text_line_input_flex_t> name) -> void {
    m_data->name_set(name->get_text());
    m_focus_list[0]->clear();
    return;
}

auto data_bi_GUI_t::data_bi_GUI_size_change_callback(pos_2D) -> void {
    if (m_focus_list.size() > 1) {
        m_focus_list[0]->clear();
    }
    else {
        auto size_menu{ new menu_t{ m_engine, m_depth_tracker, pos_2D{ get_pos().x, get_pos().y + get_size().y }
        , size_2D{ std_sideways.x, std_forward.x }, m_depth_range.near } };
        auto size_focus{ new focus_t{} };
        for (data_bi::size_t i{ data_bi::size_begin }; i < data_bi::size_end; ++i) {
            size_menu->add_option(data_bi::size_to_stringw(i)
            , std::bind(&data_bi_GUI_t::data_bi_GUI_size_set_callback, this, i, std::placeholders::_1));
        }
        size_menu->show();
        m_focus_list[0]->exclusive(std::bind(&data_bi_GUI_t::data_bi_GUI_focus_callback, this, size_menu, 1));
        m_focus_list.push_back(size_focus);
    }
    return;
}

auto data_bi_GUI_t::data_bi_GUI_size_set_callback(data_bi::size_t size, pos_2D) -> void {
    m_data->size_set(size);
    m_focus_list[0]->clear();
    return;
}

auto data_bi_GUI_t::data_bi_GUI_val_change_callback(pos_2D) -> void {
    auto val_focus{ new focus_t{} };
    auto val_div{ new divx_flex_soft_t{ m_engine, m_depth_tracker, pos_2D{ get_pos().x, get_pos().y + get_size().y }, div_create_from_member{}
    , std_forward, std_margin, alignment_2D{ alignment_x::left, alignment_y::top } } };
    auto val_input{ val_div->add_object_shared<std_text_line_input_constraint_flex_t>(m_depth_range.near, L"", std::bind(&std_text_line_input_constraint_flex_t::only_num_limit_len
    , std::placeholders::_1, std::placeholders::_2, data_bi::size_to_uint8(m_data->m_size) * 2), val_focus, std_sideways, std_white) };
    val_div->add_object<button_classic_std_t>(std::bind(&data_bi_GUI_t::data_bi_GUI_val_set_callback, this, val_input), L"確定", m_depth_range);
    m_focus_list[0]->exclusive(std::bind(&data_bi_GUI_t::data_bi_GUI_focus_callback, this, val_div, 1));
    m_focus_list.push_back(val_focus);
    return;
}

auto data_bi_GUI_t::data_bi_GUI_val_set_callback(std::shared_ptr<std_text_line_input_constraint_flex_t> val) -> void {
    m_data->val_set(val->get_text());
    m_focus_list[0]->clear();
    return;
}

auto data_bi_GUI_t::data_bi_GUI_focus_callback(GUI_object_t* object, std::size_t next_index) -> void {
    if (next_index < m_focus_list.size()) {
        m_focus_list[next_index]->clear();
        delete m_focus_list[next_index];
        m_focus_list.pop_back();
    }
    object->hide();
    delete object;
    return;
}

color_t data_bi_GUI_t::color{ std_blue };

data_bi_GUI_t::data_bi_GUI_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, focus_t* focus, size_1D size_x, depth_range_t depth_range, data_bi* data)
: m_engine{ engine }, m_depth_tracker{ depth_tracker }, m_div{ engine, depth_tracker, pos, div_create_from_member{}, std_forward, std_margin
, size_x, alignment_2D{ alignment_x::left, alignment_y::top }, false }, m_data{ data }, m_depth_range{ depth_range } {
    m_data->m_GUI = this;
    m_focus_list.push_back(focus);
    update();
    return;
}

auto data_bi_GUI_t::get_pos() const -> pos_2D {
    return m_div.get_pos();
}

auto data_bi_GUI_t::set_pos(pos_2D pos) -> void {
    return m_div.set_pos(pos);
}

auto data_bi_GUI_t::get_size() const -> size_2D {
    return m_div.get_size();
}

auto data_bi_GUI_t::get_margin() const -> size_1D {
    return m_div.get_margin();
}

auto data_bi_GUI_t::show() -> void {
    m_div.show();
    GUI_object_t::show();
    return;
}

auto data_bi_GUI_t::hide() -> void {
    GUI_object_t::hide();
    m_div.hide();
    return;
}

auto data_bi_GUI_t::update() -> void {
    m_focus_list[0]->clear();
    m_div.clear_state();
    m_div.add_object<text_click_area_t<std_text_line_flex_t>>(m_data->m_name
    , std_sideways, (m_depth_range.near + m_depth_range.far) / 2, color, std::bind(&data_bi_GUI_t::data_bi_GUI_name_change_callback, this, get_pos()));
    m_div.add_object<std_liney_t>(std_forward, color, (m_depth_range.near + m_depth_range.far) / 2);
    m_div.add_object<text_click_area_t<std_text_line_flex_t>>(m_data->m_name
    , std_sideways, (m_depth_range.near + m_depth_range.far) / 2, color, std::bind(&data_bi_GUI_t::data_bi_GUI_size_change_callback, this, get_pos()));
    m_div.add_object<std_liney_t>(std_forward, color, (m_depth_range.near + m_depth_range.far) / 2);
    m_div.add_object<text_click_area_t<std_text_line_flex_t>>(m_data->val_to_strw()
    , std_sideways, (m_depth_range.near + m_depth_range.far) / 2, color, std::bind(&data_bi_GUI_t::data_bi_GUI_val_change_callback, this, get_pos()));
    return;
}

#endif