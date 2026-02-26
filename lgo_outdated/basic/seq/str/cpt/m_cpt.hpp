#ifndef LGO_BASIC_SEQ_STR_CPT_M_CPT
#define LGO_BASIC_SEQ_STR_CPT_M_CPT

#include "../ms/cpt.hpp"
#include "../md/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_strm_cpt = is_strms_cpt<t_in> || is_strmd_cpt<t_in>;
    template<typename t_in>
    concept is_strm_t_cpt = is_t_cpt<t_in> && is_strm_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_strm_v_cpt = is_v_cpt<t_in> && is_strm_cpt<typename t_in::type>;
}

namespace lgo {
    template<typename t_in>
    concept is_strm_impl_cpt = is_strms_impl_cpt<t_in> || is_strmd_cpt<t_in>;
}

#endif