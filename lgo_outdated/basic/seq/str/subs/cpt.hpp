#ifndef LGO_BASIC_SEQ_STR_SUBS_CPT
#define LGO_BASIC_SEQ_STR_SUBS_CPT

#include "decl.hpp"

namespace lgo {
    template<typename>
    struct is_strsubs_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<typename... t_in>
    struct is_strsubs_cpt_stu<strsubs<t_in...>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_strsubs_cpt = is_strsubs_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_strsubs_t_cpt = is_t_cpt<t_in> && is_strsubs_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_strsubs_v_cpt = is_v_cpt<t_in> && is_strsubs_cpt<typename t_in::type>;
}

#endif