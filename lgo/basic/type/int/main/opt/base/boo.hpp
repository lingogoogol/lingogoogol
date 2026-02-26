#ifndef LGO_BASIC_TYPE_INT_MAIN_OPT_BASE_BOO
#define LGO_BASIC_TYPE_INT_MAIN_OPT_BASE_BOO

#include "cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_int_base_boo{ is_int_base_cpt<t_in> };
    template<typename t_in>
    using is_int_base_v = v<is_int_base_boo<t_in>>;
    template<typename t_in>
    using is_int_base_v_t = t<is_int_base_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_int_base_stu {
        using type = is_int_base_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_int_base_bind = bind<tem<is_int_base_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_int_base_t_boo{ is_int_base_t_cpt<t_in> };
    template<typename t_in>
    using is_int_base_t_v = v<is_int_base_t_boo<t_in>>;
    template<typename t_in>
    using is_int_base_t_v_t = t<is_int_base_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_int_base_t_stu {
        using type = is_int_base_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_int_base_t_bind = bind<tem<is_int_base_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_int_base_v_boo{ is_int_base_v_cpt<t_in> };
    template<typename t_in>
    using is_int_base_v_v = v<is_int_base_v_boo<t_in>>;
    template<typename t_in>
    using is_int_base_v_v_t = t<is_int_base_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_int_base_v_stu {
        using type = is_int_base_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_int_base_v_bind = bind<tem<is_int_base_v_stu>>;
}

#endif