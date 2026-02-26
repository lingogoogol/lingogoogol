#ifndef LGO_BASIC_SEQ_STR_CPT_S_CPT
#define LGO_BASIC_SEQ_STR_CPT_S_CPT

#include "d_cpt.hpp"

#include "../ms/cpt.hpp"
#include "../subs/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_strs_cpt = is_strd_cpt<t_in> || is_strms_cpt<t_in> || is_strsubs_cpt<t_in>;
    template<typename t_in>
    concept is_strs_t_cpt = is_t_cpt<t_in> && is_strs_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_strs_v_cpt = is_v_cpt<t_in> && is_strs_cpt<typename t_in::type>;
}

namespace lgo {
    template<typename t_in>
    concept is_strs_impl_cpt = is_strd_cpt<t_in> || is_strms_impl_cpt<t_in> || is_strsubs_cpt<t_in>;
}

#endif