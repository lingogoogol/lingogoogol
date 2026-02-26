#ifndef LGO_BASIC_SEQ_STR_CPT_SUB_CPT
#define LGO_BASIC_SEQ_STR_CPT_SUB_CPT

#include "../subs/cpt.hpp"
#include "../subd/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_strsub_cpt = is_strsubs_cpt<t_in> || is_strsubd_cpt<t_in>;
    template<typename t_in>
    concept is_strsub_t_cpt = is_t_cpt<t_in> && is_strsub_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_strsub_v_cpt = is_v_cpt<t_in> && is_strsub_cpt<typename t_in::type>;
}

#endif