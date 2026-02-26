#ifndef LGO_BASIC_TYPE_TEM_BIND_BOO
#define LGO_BASIC_TYPE_TEM_BIND_BOO

#include "cpt.hpp"

#include "../../boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_bind_boo{ is_bind_cpt<t_in> };
    template<typename t_in>
    using is_bind_v = v<is_bind_boo<t_in>>;
    template<typename t_in>
    using is_bind_v_t = t<is_bind_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_bind_stu {
        using type = v<is_bind_boo<t_in>>;
    };
}

LGO_EXPORT namespace lgo {
    using is_bind_bind = bind<tem<is_bind_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_bind_t_boo{ is_bind_t_cpt<t_in> };
    template<typename t_in>
    using is_bind_t_v = v<is_bind_t_boo<t_in>>;
    template<typename t_in>
    using is_bind_t_v_t = t<is_bind_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_bind_t_stu {
        using type = is_bind_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_bind_t_bind = bind<tem<is_bind_t_stu>>;
}

#endif