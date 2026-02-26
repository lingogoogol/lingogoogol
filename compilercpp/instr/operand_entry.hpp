#ifndef COMPILERCPP_INSTR_OPERAND_ENTRY
#define COMPILERCPP_INSTR_OPERAND_ENTRY

#include <cstdint>

#include "reg.hpp"

struct operand_entry {
    using type_t = reg::reg_t;

    using enum reg::reg;
    enum: type_t {
        r_begin = reg::end,
        r8 = r_begin,
        r16,
        r32,
        r64,
        r_end,
        m_begin = r_end,
        m8 = m_begin,
        m16,
        m32,
        m64,
        m128,
        m_end,
        r_m_begin = m_end,
        r_m8 = r_m_begin,
        r_m16,
        r_m32,
        r_m64,
        r_m_end,
        rel_begin = r_m_end,
        rel8 = rel_begin,
        rel16,
        rel32,
        rel_end,
        imm_begin = rel_end,
        imm8 = imm_begin,
        imm16,
        imm32,
        imm64,
        imm_end
    };

    using encoding_t = std::uint8_t;

    enum: encoding_t {
        discard,
        imm,
        ModR_M_r_m,
        ModR_M_reg
    };

    type_t m_type{};
    encoding_t m_encoding{};
};

#endif