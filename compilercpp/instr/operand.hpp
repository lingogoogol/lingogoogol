#ifndef COMPILERCPP_INSTR_OPERAND
#define COMPILERCPP_INSTR_OPERAND

#include <cstdint>
#include <iostream>

#include "../lib/.hpp"

#include "operand_entry.hpp"

class operand {
public:
    using type_t = std::uint8_t;

    enum: type_t {
        reg,
        rel8,
        rel16,
        rel32,
        m8,
        m16,
        m32,
        m64,
        m128,
        imm8,
        imm16,
        imm32,
        imm64
    };
private:
    type_t m_type{};
    std::uint64_t m_val{};

    auto val_size() -> std::uint8_t {
        switch (m_type) {
        case reg:
            return sizeof(reg::reg_t);
        case imm8:
            return 1;
        case imm16:
            return 2;
        case imm32:
            return 4;
        case imm64:
            return 8;
        default:
            log_file("operand type\n");
        }
    }
public:
    operand(std::istream& src) {
        m_type = get_integer<type_t>(src);
        m_val = get_integer<std::uint64_t>(src, val_size());
        return;
    }

    auto fit(const operand_entry& in) -> bool {
        if (m_type == reg) {
            if (m_val > reg::general_begin && m_val < reg::general_end) {
                return (m_val == in.m_type) || ((m_val - (reg::general_begin + 1)) / 8 == in.m_type - reg::end);
            }
            else {
                log_file("operand type\n");
            }
        }
        else {
            log_file("operand type\n");
        }
    }
};

#endif