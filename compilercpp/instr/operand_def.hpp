#ifndef COMPILERCPP_INSTR_OPERAND_DEF
#define COMPILERCPP_INSTR_OPERAND_DEF

#include <cstdint>
#include <iostream>

#include "../lib/.hpp"
#include "../GUI/object/.hpp"

#include "operand_entry.hpp"

class operand_t;
class operand_val_pv_t;
class operand_val_uint64_t;
class operand_GUI_t;

class operand_t {
public:
    using type_t = std::uint8_t;

    enum: type_t {
        begin,
        option_begin = begin,
        uint64_begin = option_begin,
        reg = uint64_begin,
        rel_begin,
        rel8 = rel_begin,
        rel16,
        rel32,
        rel_end,
        m_begin = rel_end,
        m8 = m_begin,
        m16,
        m32,
        m64,
        m128,
        m_end,
        imm_begin = m_end,
        imm8 = imm_begin,
        imm16,
        imm32,
        imm64,
        imm_end,
        uint64_end = imm_end,
        option_end = uint64_end,
        str_begin = option_end,
        address_begin = str_begin,
        address_end = address_begin + imm_end - rel_begin,
        str_end = address_end,
        end = str_end,
    };

    static auto type_to_stringw(type_t type) -> std::wstring;
    static auto val_to_stringw(type_t type, std::uint64_t val) -> std::wstring;
    static auto val_size(type_t type) -> std::uint8_t;
private:
    type_t m_type{};
    std::unique_ptr<operand_val_pv_t> m_val{};
    std::size_t m_index{};
    operand_GUI_t* m_GUI{};

    friend class operand_GUI_t;
    friend class instr_GUI_t;
public:
    operand_t(std::size_t index);
    operand_t(std::istream& src, std::size_t index);

    auto fit(const operand_entry& in) -> bool;

    auto get_type() -> type_t;
    auto get_val_uint64() -> std::uint64_t;
    auto get_val_str() -> std::string;
    auto set_val_uint64(std::uint64_t val) -> void;
    auto set_val_str(std::string val) -> void;
    auto set_type_val_uint64(type_t type, std::uint64_t val) -> void;
    auto set_type_val_str(type_t type, std::string val) -> void;

    auto save(std::ostream& dest) -> void;
};

class operand_val_pv_t {
public:
    virtual ~operand_val_pv_t() = default;
};

class operand_val_uint64_t: public operand_val_pv_t {
private:
    std::uint64_t m_data{};
public:
    operand_val_uint64_t(std::uint64_t data): m_data{ data } {}

    auto get() -> std::uint64_t {
        return m_data;
    }

    auto set(std::uint64_t data) -> void {
        m_data = data;
        return;
    }
};

class operand_val_str_t: public operand_val_pv_t {
private:
    std::string m_data{};
public:
    operand_val_str_t(std::string data): m_data{ data } {}

    auto get() -> std::string {
        return m_data;
    }

    auto set(std::string data) -> void {
        m_data = data;
        return;
    }
};

class operand_GUI_t: public GUI_object_t {
private:
    engine_t* m_engine{};
    depth_tracker_t* m_depth_tracker{};
    depth_range_t m_depth_range{};
    divx_flex_soft_t m_div{};
    std::vector<focus_t*> m_focus_list{};
    instr_GUI_t* m_parent{};
    operand_t* m_data{};
    std::weak_ptr<text_click_area_t<std_text_line_flex_t>> m_text{};
    
    auto operand_GUI_click_area_callback() -> void;
    auto operand_GUI_menu_operand_callback(operand_t::type_t type, focus_t* focus, pos_2D popup_pos) -> void;
    auto operand_GUI_menu_reg_type_callback(reg::type::type_t type, focus_t* focus, pos_2D popup_pos) -> void;
    auto operand_GUI_menu_reg_callback(reg::reg_t reg, pos_2D) -> void;
    auto operand_GUI_menu_address_callback(operand_t::type_t type, std::shared_ptr<std_text_line_input_flex_t> input) -> void;
    auto operand_GUI_menu_imm_callback(operand_t::type_t type, std::shared_ptr<std_text_line_input_constraint_flex_t> input) -> void;
    auto operand_GUI_focus_callback(GUI_object_t* object, std::size_t next_index) -> void;

    auto display_str() -> std::wstring;
public:
    static color_t color;

    operand_GUI_t() = default;
    operand_GUI_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, focus_t* focus, instr_GUI_t* parent, operand_t* data, depth_range_t depth_range);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;

    auto show() -> void override;
    auto hide() -> void override;
    auto update() -> void;
};

#endif