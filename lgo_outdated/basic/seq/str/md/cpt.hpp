#ifndef LGO_BASIC_SEQ_STR_MD_CPT
#define LGO_BASIC_SEQ_STR_MD_CPT

#include "decl.hpp"

#include "../../../type/tem/t/cpt.hpp"
#include "../../../type/tem/v/cpt.hpp"

namespace lgo {
    template<typename>
    struct is_strmd_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<typename... t_in>
    struct is_strmd_cpt_stu<strmd<t_in...>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_strmd_cpt = is_strmd_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_strmd_t_cpt = is_t_cpt<t_in> && is_strmd_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_strmd_v_cpt = is_v_cpt<t_in> && is_strmd_cpt<typename t_in::type>;
}

#endif