#ifndef LGO_BASIC_SEQ_STR_MAIN_CPT
#define LGO_BASIC_SEQ_STR_MAIN_CPT

#include "../cpt/m_cpt.hpp"
#include "../cpt/sub_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_str_cpt = is_strm_cpt<t_in> || is_strsub_cpt<t_in>;
    template<typename t_in>
    concept is_str_t_cpt = is_t_cpt<t_in> && is_str_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_str_v_cpt = is_v_cpt<t_in> && is_str_cpt<typename t_in::type>;
}

namespace lgo {
    template<typename t_in>
    concept is_str_impl_cpt = is_strm_impl_cpt<t_in> || is_strsub_cpt<t_in>;
}

#endif