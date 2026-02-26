#ifndef LGO_BASIC_SEQ_STR_MD_LITERAL
#define LGO_BASIC_SEQ_STR_MD_LITERAL

#include "def.hpp"
#include "alias.hpp"

LGO_EXPORT namespace lgo::inline literal {
    template<basic_literal_str t_in>
    auto operator""_strmd8() -> strmd8 {
        return strmd8::make<t_in>();
    }

    template<basic_literal_str t_in>
    auto operator""_strmd16() -> strmd16 {
        return strmd16::make<t_in>();
    }

    template<basic_literal_str t_in>
    auto operator""_strmd32() -> strmd32 {
        return strmd32::make<t_in>();
    }

    template<basic_literal_str t_in>
    auto operator""_strmdc() -> strmdc {
        return strmdc::make<t_in>();
    }
}

#endif