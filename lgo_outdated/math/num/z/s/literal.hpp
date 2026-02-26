#ifndef LGO_MATH_NUM_Z_S_LITERAL
#define LGO_MATH_NUM_Z_S_LITERAL

#include "base_impl.hpp"
#include "interface_def.hpp"

namespace lgo {
    template<is_intp_cpt t_type, is_strs_v_cpt t_in>
    struct numzs_literal_deduce {
        constexpr static intpc dot_i{ t_in::val.find(fun{ [] (t_in::type::type in) { return in == U"."_chac; } }) };
        constexpr static intpc int_len{ (dot_i - U"1"_intpc).divup(t_type::bytes() * U"2"_intpc) };
        constexpr static intpc dec_len{ (t_in::type::len() - dot_i).divup(t_type::bytes() * U"2"_intpc) };
        constexpr static intsc lit{ -dec_len };
        constexpr static intpc len{ dec_len + int_len };
        using lit_v = v<lit>;
        using len_v = v<len>;
    };
}

LGO_EXPORT namespace lgo::inline literal {
    template<literal_str t_in>
    constexpr auto operator""_numzs8() -> auto {
        constexpr static strmsc in{ t_in };
        return numzs8<typename numzs_literal_deduce<intp8, v<in>>::lit_v
        , typename numzs_literal_deduce<intp8, v<in>>::len_v>{ in };
    }

    template<literal_str t_in>
    constexpr auto operator""_numzs16() -> auto {
        constexpr static strmsc in{ t_in };
        return numzs16<typename numzs_literal_deduce<intp16, v<in>>::lit_v
        , typename numzs_literal_deduce<intp16, v<in>>::len_v>{ in };
    }

    template<literal_str t_in>
    constexpr auto operator""_numzs32() -> auto {
        constexpr static strmsc in{ t_in };
        return numzs32<typename numzs_literal_deduce<intp32, v<in>>::lit_v
        , typename numzs_literal_deduce<intp32, v<in>>::len_v>{ in };
    }

    template<literal_str t_in>
    constexpr auto operator""_numzs64() -> auto {
        constexpr static strmsc in{ t_in };
        return numzs64<typename numzs_literal_deduce<intp64, v<in>>::lit_v
        , typename numzs_literal_deduce<intp64, v<in>>::len_v>{ in };
    }

    template<literal_str t_in>
    constexpr auto operator""_numzsc() -> auto {
        constexpr static strmsc in{ t_in };
        return in.get<numzsc<typename numzs_literal_deduce<intpc, v<in>>::lit_v
        , typename numzs_literal_deduce<intpc, v<in>>::len_v>>();
    }
}

#endif