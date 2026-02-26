#ifndef LGO_BASIC_SEQ_MEM_CPT_D_CPT
#define LGO_BASIC_SEQ_MEM_CPT_D_CPT

#include "../md/cpt.hpp"
#include "../subd/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_memd_cpt = is_memmd_cpt<t_in> || is_memsubd_cpt<t_in>;
    template<typename t_in>
    concept is_memd_t_cpt = is_t_cpt<t_in> && is_memd_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_memd_v_cpt = is_v_cpt<t_in> && is_memd_cpt<typename t_in::type>;
}

#endif