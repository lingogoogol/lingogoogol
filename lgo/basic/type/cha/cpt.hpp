#ifndef LGO_BASIC_TYPE_CHA_CPT
#define LGO_BASIC_TYPE_CHA_CPT

#include "decl.hpp"
#include "basic_decl.hpp"

#include "../boo/cpt_literal.hpp"
#include "../tem/t/cpt.hpp"
#include "../tem/v/cpt.hpp"

namespace lgo {
    template<typename>
    struct is_cha_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<typename... t_in>
    struct is_cha_cpt_stu<cha<t_in...>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_cha_cpt = is_cha_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_cha_t_cpt = is_t_cpt<t_in> && is_cha_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_cha_v_cpt = is_v_cpt<t_in> && is_cha_cpt<typename t_in::type>;
}

#endif