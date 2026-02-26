#ifndef COMPILERCPP_INSTR_OPERAND_ENTRY
#define COMPILERCPP_INSTR_OPERAND_ENTRY

#include <cstdint>

#include "reg.hpp"

struct operand_entry {
    using type_t = std::uint8_t;

    enum: type_t {
        r8 = reg::end,
        r16,
        r32,
        r64,
        m8,
        m16,
        m32,
        m64,
        m128,
        r_m8,
        r_m16,
        r_m32,
        r_m64,
        rel8,
        rel16,
        rel32,
        imm8,
        imm16,
        imm32,
        imm64
    };

    using encoding_t = std::uint8_t;

    enum: encoding_t {
        imm,
        ModR_M_r_m,
        ModR_M_reg
    };

    type_t m_type{};
    encoding_t m_encoding{};
};

#endif