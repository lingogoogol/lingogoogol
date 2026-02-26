#ifndef COMPILERCPP_INSTR_REG
#define COMPILERCPP_INSTR_REG

#include <cstdint>

namespace reg {
    using reg_t = std::uint8_t;

    enum reg: reg_t {
        begin,
        general_begin = begin,
        general8_begin = general_begin,
        AL = general8_begin,
        CL,
        DL,
        BL,
        AH,
        CH,
        DH,
        BH,
        general8_end,
        general16_begin = general8_end,
        AX = general16_begin,
        CX,
        DX,
        BX,
        SP,
        BP,
        SI,
        DI,
        general16_end,
        general32_begin = general16_end,
        EAX = general32_begin,
        ECX,
        EDX,
        EBX,
        ESP,
        EBP,
        ESI,
        EDI,
        general32_end,
        general64_begin = general32_end,
        RAX = general64_begin,
        RCX,
        RDX,
        RBX,
        RSP,
        RBP,
        RSI,
        RDI,
        general64_end,
        general_end = general64_end,
        end = general_end
    };

    namespace type {
        using type_t = std::uint8_t;

        enum: type_t {
            begin,
            general8 = begin,
            general16,
            general32,
            general64,
            end
        };

        auto get_begin(type_t type) -> reg_t {
            switch (type) {
            case general8:
                return general8_begin;
            case general16:
                return general16_begin;
            case general32:
                return general32_begin;
            case general64:
                return general64_begin;
            default:
                throw error_t{ "reg type" };
            }
        }

        auto get_end(type_t type) -> reg_t {
            switch (type) {
            case general8:
                return general8_end;
            case general16:
                return general16_end;
            case general32:
                return general32_end;
            case general64:
                return general64_end;
            default:
                throw error_t{ "reg type" };
            }
        }

        auto to_stringw(type_t type) -> std::wstring {
            switch (type) {
            case general8:
                return L"General8";
            case general16:
                return L"General16";
            case general32:
                return L"General32";
            case general64:
                return L"General64";
            default:
                throw error_t{ "reg type" };
            }
        }
    }

    auto to_stringw(reg_t reg) -> std::wstring {
        switch (reg) {
        case AL:
            return L"AL";
        case CL:
            return L"CL";
        case DL:
            return L"DL";
        case BL:
            return L"BL";
        case AH:
            return L"AH";
        case CH:
            return L"CH";
        case DH:
            return L"DH";
        case BH:
            return L"BH";
        case AX:
            return L"AX";
        case CX:
            return L"CX";
        case DX:
            return L"DX";
        case BX:
            return L"BX";
        case SP:
            return L"SP";
        case BP:
            return L"BP";
        case SI:
            return L"SI";
        case DI:
            return L"DI";
        case EAX:
            return L"EAX";
        case ECX:
            return L"ECX";
        case EDX:
            return L"EDX";
        case EBX:
            return L"EBX";
        case ESP:
            return L"ESP";
        case EBP:
            return L"EBP";
        case ESI:
            return L"ESI";
        case EDI:
            return L"EDI";
        case RAX:
            return L"RAX";
        case RCX:
            return L"RCX";
        case RDX:
            return L"RDX";
        case RBX:
            return L"RBX";
        case RSP:
            return L"RSP";
        case RBP:
            return L"RBP";
        case RSI:
            return L"RSI";
        case RDI:
            return L"RDI";
        default:
            throw error_t{ "reg" };
        }
    }
}

#endif