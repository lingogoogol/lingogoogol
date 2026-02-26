#ifndef COMPILERCPP_INSTR_MNEMONIC
#define COMPILERCPP_INSTR_MNEMONIC

#include <cstdint>

namespace mnemonic {
    using mnemonic_t = std::uint16_t;

    enum: mnemonic_t {
        ADD,
        RET,
        XOR
    };
};

#endif