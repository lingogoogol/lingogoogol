#ifndef COMPILERCPP_INSTR_MAIN_IMPL
#define COMPILERCPP_INSTR_MAIN_IMPL

#include "main_def.hpp"

instr::instr(std::istream& src) {
    m_mnemonic = get_integer<mnemonic::mnemonic_t>(src);
    std::uint8_t option_len{ get_integer<std::uint8_t>(src) };
    for (std::uint8_t i{ 0 }; i < option_len; ++i) {
        m_option.push_back(get_integer<option_t>(src));
    }
    std::uint8_t operand_len{ get_integer<std::uint8_t>(src) };
    for (std::uint8_t i{ 0 }; i < operand_len; ++i) {
        m_operand.push_back(operand_t{ src, i });
    }
    return;
}

auto instr::set_mnemonic(mnemonic::mnemonic_t mnemonic) -> void {
    m_mnemonic = mnemonic;
    m_operand.clear();
    m_option.clear();
    m_GUI->update();
    return;
}

auto instr::fit(const instr_entry& in) -> bool {
    bool out{ m_operand.size() == in.m_operand.size() };
    for (std::uint8_t i{ 0 }; i < m_operand.size(); ++i) {
        out = out && m_operand[i].fit(in.m_operand[i]);
    }
    return out;
}

auto instr::save(std::ostream& dest) -> void {
    pad_integer<mnemonic::mnemonic_t>(dest, m_mnemonic);
    pad_integer(dest, static_cast<std::uint8_t>(m_option.size()));
    for (std::size_t i{ 0 }; i < m_option.size(); ++i) {
        pad_integer(dest, m_option[i]);
    }
    pad_integer(dest, static_cast<std::uint8_t>(m_operand.size()));
    for (std::size_t i{ 0 }; i < m_operand.size(); ++i) {
        m_operand[i].save(dest);
    }
    return;
}

auto instr::assemble(code_stream& dest, const instr_entry& entry) -> void {
    std::uint8_t opcode_i{ 0 };
    std::uint8_t operand_i{ 0 };
    if (entry.m_opcode[opcode_i].m_type == opcode::operand_size_16) {
        dest.pad_byte(0x66);
        ++opcode_i;
    }
    if (entry.m_opcode[opcode_i].m_type == opcode::operand_size_32) {
        ++opcode_i;
    }
    if (entry.m_opcode[opcode_i].m_type == opcode::constant) {
        dest.pad_byte(entry.m_opcode[opcode_i].m_value);
        ++opcode_i;
    }
    bool have_Mod_R_M{ false };
    unsigned char r_m{};
    if (operand_i < entry.m_operand.size() && entry.m_operand[operand_i].m_encoding == operand_entry::ModR_M_reg) {
        r_m = static_cast<unsigned char>((m_operand[operand_i].get_val_uint64() % 8) << 3);
        have_Mod_R_M = true;
        ++operand_i;
    }
    if (operand_i < entry.m_operand.size() && entry.m_operand[operand_i].m_encoding == operand_entry::ModR_M_r_m) {
        reg::reg_t val{ static_cast<reg::reg_t>(m_operand[operand_i].get_val_uint64()) };
        operand_t::type_t type{ m_operand[operand_i].get_type() };
        switch (type) {
        case operand_t::reg:
            if (val < reg::EAX || val > reg::EDI || val == reg::ESP || val == reg::EBP) {
                throw error_t{ "not supported" };
            }
            r_m |= 0b11000000 | (val - reg::EAX);
            break;
        default:
            throw error_t{ "operand_type" };
        }
        ++operand_i;
    }
    if (have_Mod_R_M) {
        dest.pad_byte(r_m);
    }
    bool rel_IP_needed{ false };
    std::uint64_t rel_ref_id{};
    std::string rel_name{};
    if (operand_i < entry.m_operand.size() && entry.m_operand[operand_i].m_encoding == operand_entry::imm) {
        operand_t::type_t type{ m_operand[operand_i].get_type() };
        switch (type) {
        case operand_t::rel8:
        case operand_t::imm8:
        case operand_t::rel16:
        case operand_t::imm16:
        case operand_t::rel32:
        case operand_t::imm32:
        case operand_t::imm64: {
            std::uint64_t val{ m_operand[operand_i].get_val_uint64() };
            for (int i{ 0 }; i < operand_t::val_size(type); ++i) {
                dest.pad_byte(static_cast<unsigned char>(val));
                val >>= 8;
            }
            break;
        }
        case operand_t::address_begin - operand_t::rel_begin + operand_t::rel8:
        case operand_t::address_begin - operand_t::rel_begin + operand_t::imm8:
        case operand_t::address_begin - operand_t::rel_begin + operand_t::rel16:
        case operand_t::address_begin - operand_t::rel_begin + operand_t::imm16:
        case operand_t::address_begin - operand_t::rel_begin + operand_t::rel32:
        case operand_t::address_begin - operand_t::rel_begin + operand_t::imm32:
        case operand_t::address_begin - operand_t::rel_begin + operand_t::imm64: {
            std::uint8_t size{ operand_t::val_size(type - operand_t::address_begin + operand_t::rel_begin) };
            code_stream::type_t address_type{};
            if (entry.m_operand[operand_i].m_type >= operand_entry::rel_begin && entry.m_operand[operand_i].m_type < operand_entry::rel_end) {
                rel_IP_needed = true;
                rel_name = m_operand[operand_i].get_val_str();
                rel_ref_id = dest.pad_address_ref(rel_name, code_stream::resolver_default());
                dest.resolver_set(rel_ref_id, code_stream::resolver_overwrite([rel_ref_id] (const code_stream::get_address_t& get_address, const std::string name) -> std::uint64_t {
                    return get_address("symbol_" + name, code_stream::RVA) - get_address("IP_" + std::to_string(rel_ref_id) + "_" + name, code_stream::RVA);
                }, size));
            }
            else {
                throw internal_error_t{ "address_type" };
            }
            for (int i{ 0 }; i < size; ++i) {
                dest.pad_byte(0);
            }
            break;
        }
        default: {
            throw error_t{ "operand_type" };
        }
        }
        ++operand_i;
    }
    if (rel_IP_needed) {
        dest.pad_address_def("IP_" + std::to_string(rel_ref_id) + "_" + rel_name);
    }
    return;
}

auto instr::assemble(code_stream& dest) -> void {
    const std::vector<instr_entry>& entry{ instr_table[m_mnemonic] };
    std::uint8_t opcode_min{ static_cast<std::uint8_t>(-1) };
    code_stream temp{};
    bool assembled{ false };
    for (std::uint8_t i{ 0 }; i < entry.size(); ++i) {
        if (fit(entry[i])) {
            if (assembled) {
                throw error_t{ "多於一個適合的指令條目" };
            }
            assemble(temp, entry[i]);
            if (temp.plain_size() < opcode_min) {
                dest.merge(temp);
            }
            assembled = true;
        }
    }
    if (!assembled) {
        throw error_t{ "沒有適合的指令條目" };
    }
    return;
}

auto instr_GUI_t::instr_GUI_menu_callback() -> void {
    if (m_focus_list.size() > 1) {
        m_focus_list[0]->clear();
    }
    else {
        auto menu_instr{ new menu_t{ m_engine, m_depth_tracker, pos_2D{ get_pos().x, get_pos().y + get_size().y }
        , size_2D{ std_sideways.x, std_forward.x }, m_depth_range.near } };
        auto focus_instr{ new focus_t{} };
        for (mnemonic::mnemonic_t i{ mnemonic::begin }; i < mnemonic::end; ++i) {
            menu_instr->add_option(mnemonic::to_stringw(i)
            , std::bind(&instr_GUI_t::instr_GUI_menu_instr_callback, this, i, focus_instr, std::placeholders::_1));
        }
        menu_instr->show();
        m_focus_list[0]->exclusive(std::bind(&instr_GUI_t::instr_GUI_focus_callback, this, menu_instr, 1));
        m_focus_list.push_back(focus_instr);
    }
    return;
}

auto instr_GUI_t::instr_GUI_menu_instr_callback(mnemonic::mnemonic_t instr, focus_t* focus, pos_2D) -> void {
    m_data->set_mnemonic(instr);
    m_focus_list[0]->clear();
    return;
}

auto instr_GUI_t::instr_GUI_focus_callback(GUI_object_t* object, std::size_t next_index) -> void {
    if (next_index < m_focus_list.size()) {
        m_focus_list[next_index]->clear();
        delete m_focus_list[next_index];
        m_focus_list.pop_back();
    }
    object->hide();
    delete object;
    return;
}

auto instr_GUI_t::instr_GUI_add_operand_callback() -> void {
    m_data->m_operand.push_back(operand_t{ m_data->m_operand.size() });
    update();
    return;
}

auto instr_GUI_t::instr_GUI_delete_operand_callback(std::size_t index) -> void {
    m_data->m_operand.erase(m_data->m_operand.begin() + index);
    for (std::size_t i{ index }; i < m_data->m_operand.size(); ++i) {
        --m_data->m_operand[i].m_index;
    }
    update();
    return;
}

color_t instr_GUI_t::color{ std_green };

instr_GUI_t::instr_GUI_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, focus_t* focus, size_1D size_x, depth_range_t depth_range, instr* data)
: m_engine{ engine }, m_depth_tracker{ depth_tracker }, m_div{ engine, depth_tracker, pos, div_create_from_member{}, std_forward, std_margin
, size_x, alignment_2D{ alignment_x::left, alignment_y::top }, false }, m_data{ data }, m_depth_range{ depth_range } {
    m_data->m_GUI = this;
    m_focus_list.push_back(focus);
    update();
    return;
}

auto instr_GUI_t::get_pos() const -> pos_2D {
    return m_div.get_pos();
}

auto instr_GUI_t::set_pos(pos_2D pos) -> void {
    return m_div.set_pos(pos);
}

auto instr_GUI_t::get_size() const -> size_2D {
    return m_div.get_size();
}

auto instr_GUI_t::get_margin() const -> size_1D {
    return m_div.get_margin();
}

auto instr_GUI_t::show() -> void {
    m_div.show();
    GUI_object_t::show();
    return;
}

auto instr_GUI_t::hide() -> void {
    GUI_object_t::hide();
    m_div.hide();
    return;
}

auto instr_GUI_t::update() -> void {
    m_focus_list[0]->clear();
    m_div.clear_state();
    m_div.add_object<text_click_area_t<std_text_line_t>>(mnemonic::to_stringw(m_data->m_mnemonic)
    , std_sideways, (m_depth_range.near + m_depth_range.far) / 2, color, std::bind(&instr_GUI_t::instr_GUI_menu_callback, this));
    m_div.add_object<std_liney_t>(std_forward, color, (m_depth_range.near + m_depth_range.far) / 2);
    for (std::size_t i{ 0 }; i < m_data->m_operand.size(); ++i) {
        m_div.add_object<operand_GUI_t>(m_focus_list[0], this, &m_data->m_operand[i], depth_range_t{ m_depth_range.near, (m_depth_range.near + m_depth_range.far) / 2 });
        m_div.add_object<std_liney_t>(std_forward, color, (m_depth_range.near + m_depth_range.far) / 2);
    }
    m_plus_id = m_div.add_object<std_button_plus_t>(depth_range_t{ (m_depth_range.near * 3 + m_depth_range.far) / 4, (m_depth_range.near + m_depth_range.far) / 2 }, color
    , std::bind(&instr_GUI_t::instr_GUI_add_operand_callback, this));
    return;
}

#endif