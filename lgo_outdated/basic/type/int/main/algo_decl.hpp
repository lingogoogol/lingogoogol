#ifndef LGO_BASIC_TYPE_INT_MAIN_ALGO_DECL
#define LGO_BASIC_TYPE_INT_MAIN_ALGO_DECL

#include "cpt.hpp"

#include "../../deduce/cond.hpp"
#include "../../fun/cmp.hpp"

namespace lgo {
    template<is_int_cpt t_in1, is_int_cpt t_in2>
    struct extre_stu {
        constexpr static boo first{ t_in1::bits() >= t_in2::bits() };
        using type = tcond<arrmc<v<first>, t_in1>, arrmc<boo::val_v<U"1">, t_in2>>;
    };

    template<is_int_cpt t_in1, is_int_cpt t_in2>
    constexpr auto min(t_in1 in1, t_in2 in2) -> extre_stu<t_in1, t_in2>::type;
    template<is_int_cpt t_in1, is_int_cpt t_in2>
    constexpr auto max(t_in1 in1, t_in2 in2) -> extre_stu<t_in1, t_in2>::type;
}

#endif