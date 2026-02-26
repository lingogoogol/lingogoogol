#ifndef LGO_BASIC_SEQ_STR_CPT_D_CPT
#define LGO_BASIC_SEQ_STR_CPT_D_CPT

#include "../md/cpt.hpp"
#include "../subd/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_strd_cpt = is_strmd_cpt<t_in> || is_strsubd_cpt<t_in>;
    template<typename t_in>
    concept is_strd_t_cpt = is_t_cpt<t_in> && is_strd_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_strd_v_cpt = is_v_cpt<t_in> && is_strd_cpt<typename t_in::type>;
}

#endif