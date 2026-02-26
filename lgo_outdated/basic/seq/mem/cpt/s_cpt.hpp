#ifndef LGO_BASIC_SEQ_MEM_CPT_S_CPT
#define LGO_BASIC_SEQ_MEM_CPT_S_CPT

#include "d_cpt.hpp"

#include "../ms/cpt.hpp"
#include "../subs/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_mems_cpt = is_memd_cpt<t_in> || is_memms_cpt<t_in> || is_memsubs_cpt<t_in>;
    template<typename t_in>
    concept is_mems_t_cpt = is_t_cpt<t_in> && is_mems_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_mems_v_cpt = is_v_cpt<t_in> && is_mems_cpt<typename t_in::type>;
}

#endif