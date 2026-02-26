#ifndef LGO_BASIC_TYPE_INT_MAIN_ALGO_IMPL
#define LGO_BASIC_TYPE_INT_MAIN_ALGO_IMPL

#include "algo_decl.hpp"
#include "impl.hpp"

namespace lgo {
    template<is_int_cpt t_in1, is_int_cpt t_in2>
    constexpr auto min(t_in1 in1, t_in2 in2) -> extre_stu<t_in1, t_in2>::type {
        if ((in1 <= in2).basic()) {
            return in1;
        }
        return in2;
    }

    template<is_int_cpt t_in1, is_int_cpt t_in2>
    constexpr auto max(t_in1 in1, t_in2 in2) -> extre_stu<t_in1, t_in2>::type {
        if ((in1 >= in2).basic()) {
            return in1;
        }
        return in2;
    }
}

#endif