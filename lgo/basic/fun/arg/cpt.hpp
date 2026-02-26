#ifndef LGO_BASIC_FUN_ARG_CPT
#define LGO_BASIC_FUN_ARG_CPT

#include "fixed/cpt.hpp"
#include "placeholder/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_fun_arg_cpt = is_fun_fixed_cpt<t_in> || is_fun_placeholder_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_fun_arg_boo{ is_fun_arg_cpt<t_in> };
    template<typename t_in>
    using is_fun_arg_v = v<is_fun_arg_boo<t_in>>;
    template<typename t_in>
    using is_fun_arg_v_t = t<is_fun_arg_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_fun_arg_stu {
        using type = is_fun_arg_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_fun_arg_bind = bind<tem<is_fun_arg_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_fun_arg_t_cpt = is_t_cpt<t_in> && is_fun_arg_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_fun_arg_t_boo{ is_fun_arg_t_cpt<t_in> };
    template<typename t_in>
    using is_fun_arg_t_v = v<is_fun_arg_t_boo<t_in>>;
    template<typename t_in>
    using is_fun_arg_t_v_t = t<is_fun_arg_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_fun_arg_t_stu {
        using type = is_fun_arg_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_fun_arg_t_bind = bind<tem<is_fun_arg_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_fun_arg_v_cpt = is_v_cpt<t_in> && is_fun_arg_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_fun_arg_v_boo{ is_fun_arg_v_cpt<t_in> };
    template<typename t_in>
    using is_fun_arg_v_v = v<is_fun_arg_v_boo<t_in>>;
    template<typename t_in>
    using is_fun_arg_v_v_t = t<is_fun_arg_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_fun_arg_v_stu {
        using type = is_fun_arg_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_fun_arg_v_bind = bind<tem<is_fun_arg_v_stu>>;
}

#endif