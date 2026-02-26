#ifndef LGO_BASIC_TYPE_INT_U_CPT
#define LGO_BASIC_TYPE_INT_U_CPT

#include "../p/cpt.hpp"
#include "../n/cpt.hpp"
#include "../../tem/t/cpt.hpp"
#include "../../tem/v/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_intu_cpt = is_intp_cpt<t_in> || is_intn_cpt<t_in>;
    template<typename t_in>
    concept is_intu_t_cpt = is_t_cpt<t_in> && is_intu_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_intu_v_cpt = is_v_cpt<t_in> && is_intu_cpt<typename t_in::type>;
}

#endif