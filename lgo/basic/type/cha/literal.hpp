#ifndef LGO_BASIC_TYPE_CHA_LITERAL
#define LGO_BASIC_TYPE_CHA_LITERAL

#include "impl.hpp"
#include "literal_decl.hpp"

LGO_EXPORT namespace lgo::inline literal {
    template<basic_literal_str t_in>
    constexpr auto operator""_cha8() -> cha8 {
        return cha8::make<t_in>();
    }

    template<basic_literal_str t_in>
    constexpr auto operator""_cha16() -> cha16 {
        return cha16::make<t_in>();
    }

    template<basic_literal_str t_in>
    constexpr auto operator""_cha32() -> cha32 {
        return cha32::make<t_in>();
    }

    template<basic_literal_str t_in>
    constexpr auto operator""_chac() -> chac {
        return chac::make<t_in>();
    }
}

#endif