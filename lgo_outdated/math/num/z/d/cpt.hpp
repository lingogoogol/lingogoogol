#ifndef LGO_MATH_NUM_Z_D_CPT
#define LGO_MATH_NUM_Z_D_CPT

#include "decl.hpp"

namespace lgo {
    template<typename>
    struct is_numzd_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_numzd_stu<numzd<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzd_bind = bind<tem<is_numzd_stu>>;
    template<typename t_in>
    using is_numzd_v = is_numzd_bind::type<t_in>;
    template<typename t_in>
    using is_numzd_v_t = t<is_numzd_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_numzd_boo{ is_numzd_v<t_in>::val };
    template<typename t_in>
    concept is_numzd_cpt = is_numzd_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_numzd_t_cpt = is_t_cpt<t_in> && is_numzd_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzd_t_boo{ is_numzd_t_cpt<t_in> };
    template<typename t_in>
    using is_numzd_t_v = v<is_numzd_t_boo<t_in>>;
    template<typename t_in>
    using is_numzd_t_v_t = t<is_numzd_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzd_t_stu {
        using type = is_numzd_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzd_t_bind = bind<tem<is_numzd_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_numzd_v_cpt = is_v_cpt<t_in> && is_numzd_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzd_v_boo{ is_numzd_v_cpt<t_in> };
    template<typename t_in>
    using is_numzd_v_v = v<is_numzd_v_boo<t_in>>;
    template<typename t_in>
    using is_numzd_v_v_t = t<is_numzd_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzd_v_stu {
        using type = is_numzd_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzd_v_bind = bind<tem<is_numzd_v_stu>>;
}

#endif