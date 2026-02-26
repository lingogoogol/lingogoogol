#ifndef COMPILERCPP_INSTR_MNEMONIC
#define COMPILERCPP_INSTR_MNEMONIC

#include <cstdint>

namespace mnemonic {
    using mnemonic_t = std::uint16_t;

    enum: mnemonic_t {
        begin,
        ADD = begin,
        CALL,
        RET,
        XOR,
        end
    };

    auto to_stringw(mnemonic_t mnemonic) -> std::wstring {
        switch (mnemonic) {
            case ADD: {
                return L"ADD";
            }
            case CALL: {
                return L"CALL";
            }
            case RET: {
                return L"RET";
            }
            case XOR: {
                return L"XOR";
            }
            default: {
                throw error_t{ "mnemonic" };
            }
        }
    }
};

#endif