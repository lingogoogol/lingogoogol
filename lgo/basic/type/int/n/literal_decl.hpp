#ifndef LGO_BASIC_TYPE_INT_N_LITERAL_DECL
#define LGO_BASIC_TYPE_INT_N_LITERAL_DECL

#include "alias.hpp"

#include "../../literal_str/basic.hpp"

namespace lgo::inline literal {
    template<basic_literal_str t_in>
    constexpr auto operator""_intn8() -> intn8;
    template<basic_literal_str t_in>
    constexpr auto operator""_intn16() -> intn16;
    template<basic_literal_str t_in>
    constexpr auto operator""_intn32() -> intn32;
    template<basic_literal_str t_in>
    constexpr auto operator""_intn64() -> intn64;
    template<basic_literal_str t_in>
    constexpr auto operator""_intnc() -> intnc;
}

#endif