#ifndef LGO_BASIC_TYPE_INT_MAIN_BOO
#define LGO_BASIC_TYPE_INT_MAIN_BOO

#include "cpt.hpp"

#include "../../boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_int_boo{ is_int_cpt<t_in> };
    template<typename t_in>
    using is_int_v = v<is_int_boo<t_in>>;
    template<typename t_in>
    using is_int_v_t = t<is_int_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_int_stu {
        using type = is_int_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_int_bind = bind<tem<is_int_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_int_t_boo{ is_int_t_cpt<t_in> };
    template<typename t_in>
    using is_int_t_v = v<is_int_t_boo<t_in>>;
    template<typename t_in>
    using is_int_t_v_t = t<is_int_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_int_t_stu {
        using type = is_int_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_int_t_bind = bind<tem<is_int_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_int_v_boo{ is_int_v_cpt<t_in> };
    template<typename t_in>
    using is_int_v_v = v<is_int_v_boo<t_in>>;
    template<typename t_in>
    using is_int_v_v_t = t<is_int_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_int_v_stu {
        using type = is_int_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_int_v_bind = bind<tem<is_int_v_stu>>;
}

#endif