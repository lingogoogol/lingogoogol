#ifndef COMPILERCPP_INSTR_REG
#define COMPILERCPP_INSTR_REG

#include <cstdint>

namespace reg {
    enum: std::uint8_t {
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