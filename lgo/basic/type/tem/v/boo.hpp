#ifndef LGO_BASIC_TYPE_TEM_V_BOO
#define LGO_BASIC_TYPE_TEM_V_BOO

#include "cpt.hpp"

#include "../../boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_v_boo{ is_v_cpt<t_in> };
    template<typename t_in>
    using is_v_v = v<is_v_boo<t_in>>;
    template<typename t_in>
    using is_v_v_t = t<is_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_v_stu {
        using type = v<is_v_boo<t_in>>;
    };
}

LGO_EXPORT namespace lgo {
    using is_v_bind = bind<tem<is_v_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_v_t_boo{ is_v_t_cpt<t_in> };
    template<typename t_in>
    using is_v_t_v = v<is_v_t_boo<t_in>>;
    template<typename t_in>
    using is_v_t_v_t = t<is_v_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_v_t_stu {
        using type = is_v_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_v_t_bind = bind<tem<is_v_t_stu>>;
}

#endif