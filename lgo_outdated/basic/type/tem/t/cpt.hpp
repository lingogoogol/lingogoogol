#ifndef LGO_BASIC_TYPE_TEM_T_CPT
#define LGO_BASIC_TYPE_TEM_T_CPT

#include "decl.hpp"

#include "../../boo/cpt.hpp"

namespace lgo {
    template<typename>
    struct is_t_cpt_stu {
        constexpr static cpt val{ false };
    };

    template<typename t_type>
    struct is_t_cpt_stu<t<t_type>> {
        constexpr static cpt val{ true };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_t_cpt = is_t_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_t_t_cpt = is_t_cpt<t_in> && is_t_cpt<typename t_in::type>;
}

#endif