#ifndef COMPILERCPP_INSTR_OPERAND_IMPL
#define COMPILERCPP_INSTR_OPERAND_IMPL

#include "operand_def.hpp"
#include "main_def.hpp"

auto operand_t::type_to_stringw(type_t type) -> std::wstring {
    switch (type) {
        case reg: {
            return L"暫存器";
        }
        case rel8: {
            return L"相對位址8";
        }
        case rel16: {
            return L"相對位址16";
        }
        case rel32: {
            return L"相對位址32";
        }
        case m8: {
            return L"記憶體8";
        }
        case m16: {
            return L"記憶體16";
        }
        case m32: {
            return L"記憶體32";
        }
        case m64: {
            return L"記憶體64";
        }
        case m128: {
            return L"記憶體128";
        }
        case imm8: {
            return L"常數8";
        }
        case imm16: {
            return L"常數16";
        }
        case imm32: {
            return L"常數32";
        }
        case imm64: {
            return L"常數64";
        }
        default: {
            if (type >= address_begin && type < address_end) {
                return type_to_stringw(type - address_begin + rel_begin) + L"位址";
            }
            throw error_t{ "operand_type" };
        }
    }
}

auto operand_t::val_to_stringw(type_t type, std::uint64_t val) -> std::wstring {
    switch (type) {
    case reg:
        return reg::to_stringw(static_cast<reg::reg_t>(val));
    case rel8:
    case rel16:
    case rel32:
    case m8:
    case m16:
    case m32:
    case m64:
    case m128:
    case imm8:
    case imm16:
    case imm32:
    case imm64:
        return to_wstring(val, val_size(type));
    default:
        throw error_t{ "operand_type" };
    }
}

auto operand_t::val_size(type_t type) -> std::uint8_t {
    switch (type) {
    case reg:
        return sizeof(reg::reg_t);
    case rel8:
    case imm8:
        return 1;
    case rel16:
    case imm16:
        return 2;
    case rel32:
    case imm32:
        return 4;
    case m8:
    case m16:
    case m32:
    case m64:
    case m128:
    case imm64:
        return 8;
    default:
        throw error_t{ "運算元種類不對" };
    }
}

operand_t::operand_t(std::size_t index): m_index{ index } {}

operand_t::operand_t(std::istream& src, std::size_t index): m_index{ index } {
    m_type = get_integer<type_t>(src);
    if (m_type >= uint64_begin && m_type < uint64_end) {
        m_val = std::make_unique<operand_val_uint64_t>(get_integer<std::uint64_t>(src, val_size(m_type)));
    }
    else if (m_type >= str_begin && m_type < str_end) {
        m_val = std::make_unique<operand_val_str_t>(get_string(src));
    }
    return;
}

auto operand_t::fit(const operand_entry& in) -> bool {
    type_t type{ m_type };
    if (type >= address_begin && type < address_end) {
        type -= address_begin;
        type += rel_begin;
    }
    if (type == reg) {
        std::uint64_t val{ get_val_uint64() };
        if (val >= reg::general_begin && val <= reg::general_end) {
            return (val == in.m_type) || ((val - reg::general_begin) / 8 == in.m_type - operand_entry::r_begin)
            || ((val - reg::general_begin) / 8 == in.m_type - operand_entry::r_m_begin);
        }
        else {
            throw error_t{ "運算元種類不對" };
        }
    }
    else if (type >= rel_begin && type < rel_end) {
        return type - rel_begin == in.m_type - operand_entry::rel_begin;
    }
    else if (type >= m_begin && type < m_end) {
        return (type - m_begin == in.m_type - operand_entry::m_begin)
        || (in.m_type < operand_entry::r_m_end && type - m_begin == in.m_type - operand_entry::r_m_begin);
    }
    else if (type > imm_begin && type < imm_end) {
        return type - imm_begin == in.m_type - operand_entry::imm_begin;
    }
    else {
        throw error_t{ "運算元種類不對" };
    }
}

auto operand_t::get_type() -> type_t {
    return m_type;
}

auto operand_t::get_val_uint64() -> std::uint64_t {
    return dynamic_cast<operand_val_uint64_t*>(m_val.get())->get();
}

auto operand_t::get_val_str() -> std::string {
    return dynamic_cast<operand_val_str_t*>(m_val.get())->get();
}

auto operand_t::set_val_uint64(std::uint64_t val) -> void {
    dynamic_cast<operand_val_uint64_t*>(m_val.get())->set(val);
    m_GUI->update();
    return;
}

auto operand_t::set_val_str(std::string val) -> void {
    dynamic_cast<operand_val_str_t*>(m_val.get())->set(val);
    m_GUI->update();
    return;
}

auto operand_t::set_type_val_uint64(type_t type, std::uint64_t val) -> void {
    m_type = type;
    m_val = std::make_unique<operand_val_uint64_t>(val);
    m_GUI->update();
    return;
}

auto operand_t::set_type_val_str(type_t type, std::string val) -> void {
    m_type = type;
    m_val = std::make_unique<operand_val_str_t>(val);
    m_GUI->update();
    return;
}

auto operand_t::save(std::ostream& dest) -> void {
    pad_integer(dest, m_type);
    if (m_type >= uint64_begin && m_type < uint64_end) {
        pad_integer(dest, get_val_uint64(), val_size(m_type));
    }
    else if (m_type >= str_begin && m_type < str_end) {
        pad_string(dest, get_val_str());
    }
    return;
}

auto operand_GUI_t::operand_GUI_click_area_callback() -> void {
    if (m_focus_list.size() > 1) {
        m_focus_list[0]->clear();
    }
    else {
        auto menu_operand{ new menu_t{ m_engine, m_depth_tracker, pos_2D{ get_pos().x, get_pos().y + get_size().y }
        , size_2D{ std_sideways.x, std_forward.x }, m_depth_range.near } };
        auto focus_operand{ new focus_t{} };
        for (operand_t::type_t i{ operand_t::option_begin }; i < operand_t::option_end; ++i) {
            menu_operand->add_option(operand_t::type_to_stringw(i)
            , std::bind(&operand_GUI_t::operand_GUI_menu_operand_callback, this, i, focus_operand, std::placeholders::_1));
        }
        menu_operand->add_option(L"刪除"
        , std::bind(&instr_GUI_t::instr_GUI_delete_operand_callback, m_parent, m_data->m_index));
        menu_operand->show();
        m_focus_list[0]->exclusive(std::bind(&operand_GUI_t::operand_GUI_focus_callback, this, menu_operand, 1));
        m_focus_list.push_back(focus_operand);
    }
    return;
}

auto operand_GUI_t::operand_GUI_menu_operand_callback(operand_t::type_t type, focus_t* focus, pos_2D popup_pos) -> void {
    divy_flex_soft_t* div{};
    auto div_create{ [this, &div, popup_pos] () -> void {
        div = new divy_flex_soft_t{ m_engine, m_depth_tracker, popup_pos, div_create_from_member{}
        , std_sideways, std_margin, alignment_2D{ alignment_x::left, alignment_y::top } };
        return;
    } };
    focus_t* focus_option{ new focus_t{} };
    switch (type) {
        case operand_t::reg: {
            menu_t* menu_reg_type{ new menu_t{ m_engine, m_depth_tracker, popup_pos, size_2D{ std_sideways.x, std_forward.x }, m_depth_range.near } };
            for (reg::type::type_t i{ reg::type::begin }; i < reg::type::end; ++i) {
                menu_reg_type->add_option(reg::type::to_stringw(i), std::bind(&operand_GUI_t::operand_GUI_menu_reg_type_callback
                , this, i, focus_option, std::placeholders::_1));
            }
            menu_reg_type->show();
            focus->exclusive(std::bind(&operand_GUI_t::operand_GUI_focus_callback, this, menu_reg_type, 2));
            m_focus_list.push_back(focus_option);
            break;
        }
        case operand_t::rel8:
        case operand_t::rel16:
        case operand_t::rel32:
        case operand_t::m8:
        case operand_t::m16:
        case operand_t::m32:
        case operand_t::m64:
        case operand_t::m128:
        case operand_t::imm8:
        case operand_t::imm16:
        case operand_t::imm32:
        case operand_t::imm64: {
            div_create();
            auto address_div{ div->add_object_weak<divx_flex_soft_t>(div_create_from_member{}
            , std_forward, std_margin, alignment_2D{ alignment_x::left, alignment_y::top }) };
            address_div.lock()->add_object<std_text_line_t>(L"值：", std_sideways, std_white);
            auto address_input{ address_div.lock()->add_object_shared<std_text_line_input_constraint_flex_t>(m_depth_range.near, L""
            , std::bind(&std_text_line_input_constraint_flex_t::only_num_limit_len
            , std::placeholders::_1, std::placeholders::_2, operand_t::val_size(type) * 2), focus_option, std_sideways, std_white) };
            address_div.lock()->add_object<button_classic_std_t>(std::bind(&operand_GUI_t::operand_GUI_menu_imm_callback, this, type, address_input), L"確定", m_depth_range);
            auto symbol_div{ div->add_object_weak<divx_flex_soft_t>(div_create_from_member{}
            , std_forward, std_margin, alignment_2D{ alignment_x::left, alignment_y::top }) };
            symbol_div.lock()->add_object<std_text_line_t>(L"位址：", std_sideways, std_white);
            auto symbol_input{ symbol_div.lock()->add_object_shared<std_text_line_input_flex_t>(m_depth_range.near, L"", focus_option, std_sideways, std_white) };
            symbol_div.lock()->add_object<button_classic_std_t>(std::bind(&operand_GUI_t::operand_GUI_menu_address_callback
            , this, static_cast<operand_t::type_t>(operand_t::address_begin + type - operand_t::rel_begin), symbol_input), L"確定", m_depth_range);
            div->show();
            focus->exclusive(std::bind(&operand_GUI_t::operand_GUI_focus_callback, this, div, 2));
            m_focus_list.push_back(focus_option);
            break;
        }
        default: {
            throw error_t{ "operand type" };
        }
    }
    return;
}

auto operand_GUI_t::operand_GUI_menu_reg_type_callback(reg::type::type_t type, focus_t* focus, pos_2D popup_pos) -> void {
    menu_t* menu_reg{ new menu_t{ m_engine, m_depth_tracker, popup_pos, size_2D{ std_sideways.x, std_forward.x }, m_depth_range.near } };
    for (reg::reg_t i{ reg::type::get_begin(type) }; i < reg::type::get_end(type); ++i) {
        menu_reg->add_option(reg::to_stringw(i), std::bind(&operand_GUI_t::operand_GUI_menu_reg_callback, this, i, std::placeholders::_1));
    }
    menu_reg->show();
    focus->exclusive(std::bind(&operand_GUI_t::operand_GUI_focus_callback, this, menu_reg, 3));
    return;
}

auto operand_GUI_t::operand_GUI_menu_reg_callback(reg::reg_t reg, pos_2D) -> void {
    m_data->set_type_val_uint64(operand_t::reg, reg);
    m_focus_list[0]->clear();
    return;
}

auto operand_GUI_t::operand_GUI_menu_address_callback(operand_t::type_t type, std::shared_ptr<std_text_line_input_flex_t> input) -> void {
    m_data->set_type_val_str(type, to_string(input->get_text()));
    m_focus_list[0]->clear();
    return;
}

auto operand_GUI_t::operand_GUI_menu_imm_callback(operand_t::type_t type, std::shared_ptr<std_text_line_input_constraint_flex_t> input) -> void {
    const std::wstring input_str{ input->get_text() };
    if (input_str.empty()) {
        return;
    }
    m_data->set_type_val_uint64(type, to_int<std::uint64_t>(input_str));
    m_focus_list[0]->clear();
    return;
}

auto operand_GUI_t::operand_GUI_focus_callback(GUI_object_t* object, std::size_t next_index) -> void {
    if (next_index < m_focus_list.size()) {
        m_focus_list[next_index]->clear();
        delete m_focus_list[next_index];
        m_focus_list.pop_back();
    }
    object->hide();
    delete object;
    return;
}

auto operand_GUI_t::display_str() -> std::wstring {
    if (!m_data->m_val) {
        return L"選擇運算元";
    }
    std::wstring val_strw{};
    if (m_data->m_type >= operand_t::uint64_begin && m_data->m_type < operand_t::uint64_end) {
        val_strw = operand_t::val_to_stringw(m_data->m_type, m_data->get_val_uint64());
    }
    else if (m_data->m_type >= operand_t::str_begin && m_data->m_type < operand_t::str_end) {
        std::string val_str{ m_data->get_val_str() };
        val_strw = std::wstring{ val_str.begin(), val_str.end() };
    }
    return operand_t::type_to_stringw(m_data->m_type) + L"：" + val_strw;
}

color_t operand_GUI_t::color{ std_green };

operand_GUI_t::operand_GUI_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, focus_t* focus, instr_GUI_t* parent, operand_t* data, depth_range_t depth_range)
: m_engine{ engine }, m_depth_tracker{ depth_tracker }, m_parent{ parent }, m_depth_range{ depth_range }
, m_div{ engine, m_depth_tracker, pos, div_create_from_member{}, std_forward, std_margin, alignment_2D{ alignment_x::left, alignment_y::top } }, m_data{ data } {
    data->m_GUI = this;
    m_text = m_div.add_object_weak<text_click_area_t<std_text_line_flex_t>>(display_str(), std_sideways
    , depth_range.near, color, std::bind(&operand_GUI_t::operand_GUI_click_area_callback, this));
    m_div.set_size_callback(std::bind(&GUI_object_t::set_size, this, std::placeholders::_1));
    m_focus_list.push_back(focus);
    return;
}

auto operand_GUI_t::get_pos() const -> pos_2D {
    return m_div.get_pos();
}

auto operand_GUI_t::set_pos(pos_2D pos) -> void {
    m_div.set_pos(pos);
    m_focus_list[0]->clear();
    return;
}

auto operand_GUI_t::get_size() const -> size_2D {
    return m_div.get_size();
}

auto operand_GUI_t::get_margin() const -> size_1D {
    return m_div.get_margin();
}

auto operand_GUI_t::update() -> void {
    m_text.lock()->set_text(display_str());
    return;
}

auto operand_GUI_t::show() -> void {
    m_div.show();
    GUI_object_t::show();
    return;
}

auto operand_GUI_t::hide() -> void {
    GUI_object_t::hide();
    m_div.hide();
    m_focus_list[0]->clear();
    return;
}

#endif