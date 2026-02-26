#ifndef LGO_BASIC_FUN_ARG_FIXED_CPT
#define LGO_BASIC_FUN_ARG_FIXED_CPT

#include "def.hpp"

namespace lgo {
    template<typename>
    struct is_fun_fixed_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_fun_fixed_stu<fun_fixed<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_fun_fixed_bind = bind<tem<is_fun_fixed_stu>>;
    template<typename t_in>
    using is_fun_fixed_v = is_fun_fixed_bind::type<t_in>;
    template<typename t_in>
    using is_fun_fixed_v_t = t<is_fun_fixed_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_fun_fixed_boo{ is_fun_fixed_v<t_in>::val };
    template<typename t_in>
    concept is_fun_fixed_cpt = is_fun_fixed_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_fun_fixed_t_cpt = is_t_cpt<t_in> && is_fun_fixed_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_fun_fixed_t_boo{ is_fun_fixed_t_cpt<t_in> };
    template<typename t_in>
    using is_fun_fixed_t_v = v<is_fun_fixed_t_boo<t_in>>;
    template<typename t_in>
    using is_fun_fixed_t_v_t = t<is_fun_fixed_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_fun_fixed_t_stu {
        using type = is_fun_fixed_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_fun_fixed_t_bind = bind<tem<is_fun_fixed_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_fun_fixed_v_cpt = is_v_cpt<t_in> && is_fun_fixed_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_fun_fixed_v_boo{ is_fun_fixed_v_cpt<t_in> };
    template<typename t_in>
    using is_fun_fixed_v_v = v<is_fun_fixed_v_boo<t_in>>;
    template<typename t_in>
    using is_fun_fixed_v_v_t = t<is_fun_fixed_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_fun_fixed_v_stu {
        using type = is_fun_fixed_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_fun_fixed_v_bind = bind<tem<is_fun_fixed_v_stu>>;
}

#endif