#ifndef LGO_BASIC_TYPE_INT_MAIN_CPT
#define LGO_BASIC_TYPE_INT_MAIN_CPT

#include "../u/cpt.hpp"

#include "../s/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_int_cpt = is_intu_cpt<t_in> || is_ints_cpt<t_in>;
    template<typename t_in>
    concept is_int_t_cpt = is_t_cpt<t_in> && is_int_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_int_v_cpt = is_v_cpt<t_in> && is_int_cpt<typename t_in::type>;
}

#endif