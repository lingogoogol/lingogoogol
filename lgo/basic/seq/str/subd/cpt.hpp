#ifndef LGO_BASIC_SEQ_STR_SUBD_CPT
#define LGO_BASIC_SEQ_STR_SUBD_CPT

#include "decl.hpp"

namespace lgo {
    template<typename>
    struct is_strsubd_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<typename... t_in>
    struct is_strsubd_cpt_stu<strsubd<t_in...>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_strsubd_cpt = is_strsubd_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_strsubd_t_cpt = is_t_cpt<t_in> && is_strsubd_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_strsubd_v_cpt = is_v_cpt<t_in> && is_strsubd_cpt<typename t_in::type>;
}

#endif