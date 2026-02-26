#ifndef LGO_BASIC_TYPE_CHA_LITERAL_DECL
#define LGO_BASIC_TYPE_CHA_LITERAL_DECL

#include "alias.hpp"

#include "../literal_str/basic.hpp"

namespace lgo::inline literal {
    template<basic_literal_str t_in>
    constexpr auto operator""_cha8() -> cha8;
    template<basic_literal_str t_in>
    constexpr auto operator""_cha16() -> cha16;
    template<basic_literal_str t_in>
    constexpr auto operator""_cha32() -> cha32;
    template<basic_literal_str t_in>
    constexpr auto operator""_chac() -> chac;
}

#endif