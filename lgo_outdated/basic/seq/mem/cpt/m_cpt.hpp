#ifndef LGO_BASIC_SEQ_MEM_CPT_M_CPT
#define LGO_BASIC_SEQ_MEM_CPT_M_CPT

#include "../ms/cpt.hpp"
#include "../md/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_memm_cpt = is_memms_cpt<t_in> || is_memmd_cpt<t_in>;
    template<typename t_in>
    concept is_memm_t_cpt = is_t_cpt<t_in> && is_memm_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_memm_v_cpt = is_v_cpt<t_in> && is_memm_cpt<typename t_in::type>;
}

#endif