#ifndef LGO_BASIC_TYPE_INT_N_CPT
#define LGO_BASIC_TYPE_INT_N_CPT

#include "decl.hpp"

namespace lgo {
    template<typename>
    struct is_intn_cpt_stu {
        constexpr static cpt val{ false };
    };

    template<typename... t_in>
    struct is_intn_cpt_stu<intn_class<t_in...>> {
        constexpr static cpt val{ true };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_intn_cpt = is_intn_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_intn_t_cpt = is_t_cpt<t_in> && is_intn_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_intn_v_cpt = is_v_cpt<t_in> && is_intn_cpt<typename t_in::type>;
}

#endif