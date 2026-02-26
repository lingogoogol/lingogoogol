#ifndef COMPILERCPP_INSTR_OPERAND
#define COMPILERCPP_INSTR_OPERAND

#include <cstdint>

#include "../lib/.hpp"

#include "operand_entry.hpp"

struct operand {
    using type_t = std::uint8_t;

    enum: type_t {
        reg,
        imm8,
        imm16,
        imm32,
        imm64
    };

    type_t m_type{};
    std::uint64_t m_val{};

    auto fit(const operand_entry& in) -> bool {
        if (m_type == reg) {
            if (m_val > reg::general_begin && m_val < reg::general_end) {
                return (m_val == in.m_type) || ((m_val - (reg::general_begin + 1)) / 8 == in.m_type - reg::end);
            }
            else {
                throw internal_error{ "operand type" };
            }
        }
        else {
            throw internal_error{ "operand type" };
        }
    }
};

#endif