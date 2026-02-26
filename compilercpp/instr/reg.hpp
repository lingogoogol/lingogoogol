#ifndef COMPILERCPP_INSTR_REG
#define COMPILERCPP_INSTR_REG

#include <cstdint>

namespace reg {
    using reg_t = std::uint8_t;

    enum: reg_t {
        general_begin,
        AL,
        CL,
        DL,
        BL,
        AH,
        CH,
        DH,
        BH,
        AX,
        CX,
        DX,
        BX,
        SP,
        BP,
        SI,
        DI,
        EAX,
        ECX,
        EDX,
        EBX,
        ESP,
        EBP,
        ESI,
        EDI,
        RAX,
        RCX,
        RDX,
        RBX,
        RSP,
        RBP,
        RSI,
        RDI,
        general_end,
        end
    };
}

#endif