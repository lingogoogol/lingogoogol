#ifndef COMPILERCPP_INSTR_MAIN_DEF
#define COMPILERCPP_INSTR_MAIN_DEF

#include <vector>
#include <cstdint>
#include <iostream>

#include "../lib/.hpp"
#include "../stmt/pv.hpp"
#include "../GUI/object/.hpp"
#include "../code/stream.hpp"

#include "table.hpp"
#include "operand_def.hpp"

class instr;
class instr_GUI_t;

class instr: public stmt_pv {
public:
    using option_t = std::uint8_t;

    enum: option_t {};
private:
    mnemonic::mnemonic_t m_mnemonic{};
    std::vector<option_t> m_option{};
    std::vector<operand_t> m_operand{};
    instr_GUI_t* m_GUI{};

    friend class instr_GUI_t;
public:
    instr(std::istream& src);

    auto set_mnemonic(mnemonic::mnemonic_t mnemonic) -> void;

    auto fit(const instr_entry& in) -> bool;
    auto save(std::ostream& dest) -> void;
    auto assemble(code_stream& dest, const instr_entry& entry) -> void;
    auto assemble(code_stream& dest) -> void;
};

class instr_GUI_t: public GUI_object_t {
private:
    divx_soft_t m_div{};
    instr* m_data{};
    engine_t* m_engine{};
    depth_tracker_t* m_depth_tracker{};
    depth_range_t m_depth_range{};
    std::vector<focus_t*> m_focus_list{};
    std::uint64_t m_plus_id{};

    auto instr_GUI_menu_callback() -> void;
    auto instr_GUI_menu_instr_callback(mnemonic::mnemonic_t instr, focus_t* focus, pos_2D) -> void;
    auto instr_GUI_focus_callback(GUI_object_t* object, std::size_t next_index) -> void;
    auto instr_GUI_add_operand_callback() -> void;
    auto instr_GUI_delete_operand_callback(std::size_t index) -> void;

    friend class operand_GUI_t;
public:
    static color_t color;

    instr_GUI_t() = default;
    instr_GUI_t(engine_t* engine, depth_tracker_t* depth_tracker, pos_2D pos, focus_t* focus, size_1D size_x, depth_range_t depth_range, instr* data);

    auto get_pos() const -> pos_2D override;
    auto set_pos(pos_2D pos) -> void override;
    auto get_size() const -> size_2D override;
    auto get_margin() const -> size_1D override;

    auto show() -> void override;
    auto hide() -> void override;
    auto update() -> void;
};

#endif