#ifndef LGO_BASIC_SEQ_MEM_MAIN_CPT
#define LGO_BASIC_SEQ_MEM_MAIN_CPT

#include "../cpt/m_cpt.hpp"
#include "../cpt/sub_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_mem_cpt = is_memm_cpt<t_in> || is_memsub_cpt<t_in>;
    template<typename t_in>
    concept is_mem_t_cpt = is_t_cpt<t_in> && is_mem_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_mem_v_cpt = is_v_cpt<t_in> && is_mem_cpt<typename t_in::type>;
}

#endif