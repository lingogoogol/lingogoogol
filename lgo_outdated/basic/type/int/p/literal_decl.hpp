#ifndef LGO_BASIC_TYPE_INT_P_LITERAL_DECL
#define LGO_BASIC_TYPE_INT_P_LITERAL_DECL

#include "alias.hpp"

#include "../../literal_str/basic.hpp"

namespace lgo::inline literal {
    template<basic_literal_str t_in>
    constexpr auto operator""_intp8() -> intp8;
    template<basic_literal_str t_in>
    constexpr auto operator""_intp16() -> intp16;
    template<basic_literal_str t_in>
    constexpr auto operator""_intp32() -> intp32;
    template<basic_literal_str t_in>
    constexpr auto operator""_intp64() -> intp64;
    template<basic_literal_str t_in>
    constexpr auto operator""_intpc() -> intpc;
}

#endif