#ifndef LGO_BASIC_TYPE_INT_P_CPT
#define LGO_BASIC_TYPE_INT_P_CPT

#include "decl.hpp"
#include "basic.hpp"

#include "../../boo/cpt.hpp"
#include "../../tem/t/cpt.hpp"
#include "../../tem/v/cpt.hpp"

namespace lgo {
    template<typename>
    struct is_intp_cpt_stu {
        constexpr static cpt val{ false };
    };

    template<typename... t_in>
    struct is_intp_cpt_stu<intp_class<t_in...>> {
        constexpr static cpt val{ true };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_intp_cpt = is_intp_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_intp_t_cpt = is_t_cpt<t_in> && is_intp_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_intp_v_cpt = is_v_cpt<t_in> && is_intp_cpt<typename t_in::type>;
}

#endif