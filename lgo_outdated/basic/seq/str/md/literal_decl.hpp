#ifndef LGO_BASIC_SEQ_STR_MD_LITERAL_DECL
#define LGO_BASIC_SEQ_STR_MD_LITERAL_DECL

#include "alias.hpp"

LGO_EXPORT namespace lgo::inline literal {
    template<basic_literal_str t_in>
    auto operator""_strmd8() -> strmd8;
    template<basic_literal_str t_in>
    auto operator""_strmd16() -> strmd16;
    template<basic_literal_str t_in>
    auto operator""_strmd32() -> strmd32;
    template<basic_literal_str t_in>
    auto operator""_strmdc() -> strmdc;
}

#endif