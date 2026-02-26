#ifndef LGO_BASIC_TYPE_INT_S_BASIC
#define LGO_BASIC_TYPE_INT_S_BASIC

#include "../p/basic.hpp"
#include "../../constraint/common.hpp"

namespace lgo {
    using basic_ints8 = std::int8_t;
    using basic_ints8_t = t<basic_ints8>;
    using basic_ints16 = std::int16_t;
    using basic_ints16_t = t<basic_ints16>;
    using basic_ints32 = std::int32_t;
    using basic_ints32_t = t<basic_ints32>;
    using basic_ints64 = std::int64_t;
    using basic_ints64_t = t<basic_ints64>;

    using basic_intsc = basic_ints64;
    using basic_intsc_t = t<basic_intsc>;

    template<typename t_in>
    concept is_basic_ints_cpt = is_one_of_cpt<t_in, char, short, int, long, long long>;
    template<typename t_in>
    concept is_basic_ints_t_cpt = is_t_cpt<t_in> && is_basic_ints_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_basic_int_cpt = is_basic_intp_cpt<t_in> || is_basic_ints_cpt<t_in>;
    template<typename t_in>
    concept is_basic_int_t_cpt = is_t_cpt<t_in> && is_basic_int_cpt<typename t_in::type>;
}

#endif