#ifndef COMPILERCPP_INSTR_MAIN
#define COMPILERCPP_INSTR_MAIN

#include <vector>
#include <cstdint>
#include <iostream>

#include "../lib/.hpp"
#include "../stmt/pv.hpp"

#include "table.hpp"
#include "operand.hpp"

class instr: public stmt_pv {
public:
    using option_t = std::uint8_t;

    enum: option_t {};
private:
    mnemonic::mnemonic_t m_mnemonic{};
    std::vector<option_t> m_option{};
    std::vector<operand> m_operand{};
public:
    instr(std::istream& src) {
        m_mnemonic = get_integer<mnemonic::mnemonic_t>(src);
        std::uint8_t option_len{ get_integer<std::uint8_t>(src) };
        for (std::uint8_t i{ 0 }; i < option_len; ++i) {
            m_option.push_back(get_integer<option_t>(src));
        }
        std::uint8_t operand_len{ get_integer<std::uint8_t>(src) };
        for (std::uint8_t i{ 0 }; i < operand_len; ++i) {
            m_operand.push_back(operand{ src });
        }
        return;
    }

    auto fit(const instr_entry& in) -> bool {
        bool out{ m_operand.size() == in.m_operand.size() };
        for (std::uint8_t i{ 0 }; i < m_operand.size(); ++i) {
            out = out && m_operand[i].fit(in.m_operand[i]);
        }
        return out;
    }

    auto assemble(std::vector<unsigned char>& dest, const instr_entry& entry) -> void {
        dest.clear();
        std::uint8_t opcode_i{ 0 };
        if (entry.m_opcode[opcode_i].m_type == opcode::operand_size_16) {
            dest.push_back(0x66);
            ++opcode_i;
        }
        if (entry.m_opcode[opcode_i].m_type == opcode::constant) {
            dest.push_back(entry.m_opcode[opcode_i].m_value);
        }
        return;
    }
    
    auto assemble(std::vector<unsigned char>& dest) -> void {
        const std::vector<instr_entry>& entry{ instr_table[m_mnemonic] };
        std::uint8_t opcode_min{ static_cast<std::uint8_t>(-1) };
        std::vector<unsigned char> temp{};
        for (std::uint8_t i{ 0 }; i < entry.size(); ++i) {
            if (fit(entry[i])) {
                assemble(temp, entry[i]);
                if (temp.size() < opcode_min) {
                    dest = temp;
                }
            }
        }
        return;
    }
};

#endif