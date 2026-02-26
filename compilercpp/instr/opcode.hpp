#ifndef COMPILERCPP_INSTR_OPCODE
#define COMPILERCPP_INSTR_OPCODE

#include <cstdint>

struct opcode {
    using type_t = std::uint8_t;

    enum: type_t {
        operand_size_16,
        operand_size_32,
        REX_W,
        VEX_L,
        VEX_0x66,
        VEX_0xF2,
        VEX_0xF3,
        VEX_0x0F,
        VEX_0x0F3A,
        VEX_0x0F38,
        VEX_W,
        EVEX_128,
        EVEX_256,
        EVEX_512,
        EVEX_0x66,
        EVEX_0xF2,
        EVEX_0xF3,
        EVEX_0x0F,
        EVEX_0x0F3A,
        EVEX_0x0F38,
        EVEX_W,
        constant,
        constant_STi,
        ModR_M_reg_constant
    };
    
    type_t m_type{};
    std::uint8_t m_value{};
};

#endif