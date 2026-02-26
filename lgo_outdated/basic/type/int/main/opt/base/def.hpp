#ifndef LGO_BASIC_TYPE_INT_MAIN_OPT_BASE_DEF
#define LGO_BASIC_TYPE_INT_MAIN_OPT_BASE_DEF

#include "../../../p/cpt.hpp"
#include "../../../../fun/empty.hpp"

LGO_EXPORT namespace lgo {
    template<is_intp_v_cpt t_in>
    class int_base: public lgo, public empty {
    public:
        constexpr static auto base() -> intpc;
    };

    template<is_intp_v_cpt t_in>
    constexpr auto int_base<t_in>::base() -> intpc {
        return t_in::val;
    }
}

#endif