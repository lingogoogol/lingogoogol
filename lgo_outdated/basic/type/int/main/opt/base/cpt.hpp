#ifndef LGO_BASIC_TYPE_INT_MAIN_OPT_BASE_CPT
#define LGO_BASIC_TYPE_INT_MAIN_OPT_BASE_CPT

#include "def.hpp"

namespace lgo {
    template<typename>
    struct is_int_base_cpt_stu {
        constexpr static cpt val{ false };
    };

    template<typename... t_in>
    struct is_int_base_cpt_stu<int_base<t_in...>> {
        constexpr static cpt val{ true };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_int_base_cpt = is_int_base_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_int_base_t_cpt = is_t_cpt<t_in> && is_int_base_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_int_base_v_cpt = is_v_cpt<t_in> && is_int_base_cpt<typename t_in::type>;
}

#endif