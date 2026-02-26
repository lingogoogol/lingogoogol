#ifndef LGO_BASIC_TYPE_TEM_BIND_CPT
#define LGO_BASIC_TYPE_TEM_BIND_CPT

#include "decl.hpp"
#include "../t/cpt.hpp"

#include "../../boo/cpt_literal.hpp"

namespace lgo {
    template<typename>
    struct is_bind_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };
    
    template<typename... t_in>
    struct is_bind_cpt_stu<bind<t_in...>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_bind_cpt = is_bind_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_bind_t_cpt = is_t_cpt<t_in> && is_bind_cpt<typename t_in::type>;
}

#endif