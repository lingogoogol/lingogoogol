#ifndef LGO_BASIC_SEQ_MEM_MD_CPT
#define LGO_BASIC_SEQ_MEM_MD_CPT

#include "decl.hpp"

namespace lgo {
    template<typename>
    struct is_memmd_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<>
    struct is_memmd_cpt_stu<memmd> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_memmd_cpt = is_memmd_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_memmd_t_cpt = is_t_cpt<t_in> && is_memmd_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_memmd_v_cpt = is_v_cpt<t_in> && is_memmd_cpt<typename t_in::type>;
}

#endif