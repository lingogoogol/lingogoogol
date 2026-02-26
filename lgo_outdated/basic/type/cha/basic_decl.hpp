#ifndef LGO_BASIC_TYPE_CHA_BASIC_DECL
#define LGO_BASIC_TYPE_CHA_BASIC_DECL

#include "../constraint/common.hpp"
#include "../int/p/basic.hpp"
#include "../fun/cast_impl.hpp"
#include "../tem/t/cpt.hpp"

namespace lgo {
    using basic_cha8 = char8_t;
    using basic_cha8_t = t<basic_cha8>;
    using basic_cha16 = char16_t;
    using basic_cha16_t = t<basic_cha16>;
    using basic_cha32 = char32_t;
    using basic_cha32_t = t<basic_cha32>;

    using basic_chac = basic_cha32;
    using basic_chac_t = t<basic_chac>;

    template<typename t_in>
    concept is_basic_cha_cpt = is_one_of_cpt<t_in, char, char8_t, char16_t, char32_t>;
    template<typename t_in>
    concept is_basic_cha_t_cpt = is_t_cpt<t_in> && is_basic_cha_cpt<typename t_in::type>;

    template<is_basic_intp_cpt t_out, is_basic_cha_cpt t_in>
    constexpr auto to_basic_intp(t_in in) -> t_out;
    template<is_basic_cha_cpt t_out, is_basic_intp_cpt t_in>
    constexpr auto to_basic_cha(t_in in) -> t_out;
}

#endif