#ifndef LGO_BASIC_SEQ_MEM_CPT_SUB_CPT
#define LGO_BASIC_SEQ_MEM_CPT_SUB_CPT

#include "../subs/cpt.hpp"
#include "../subd/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_memsub_cpt = is_memsubs_cpt<t_in> || is_memsubd_cpt<t_in>;
    template<typename t_in>
    concept is_memsub_t_cpt = is_t_cpt<t_in> && is_memsub_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_memsub_v_cpt = is_v_cpt<t_in> && is_memsub_cpt<typename t_in::type>;
}

#endif