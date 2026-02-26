#ifndef LGO_BASIC_TYPE_TEM_V_CPT
#define LGO_BASIC_TYPE_TEM_V_CPT

#include "def.hpp"
#include "../t/cpt.hpp"

#include "../../boo/cpt.hpp"

namespace lgo {
    template<typename>
    struct is_v_cpt_stu {
        constexpr static cpt val{ false };
    };
    
    template<const auto& t_v>
    struct is_v_cpt_stu<v<t_v>> {
        constexpr static cpt val{ true };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_v_cpt = is_v_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_v_t_cpt = is_t_cpt<t_in> && is_v_cpt<typename t_in::type>;
}

#endif