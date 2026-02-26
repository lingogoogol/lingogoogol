#ifndef LGO_BASIC_TYPE_TEM_T_BOO
#define LGO_BASIC_TYPE_TEM_T_BOO

#include "cpt.hpp"

#include "../../boo/.hpp"
#include "../tem/def.hpp"
#include "../bind/def.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_t_boo{ is_t_cpt<t_in> };
    template<typename t_in>
    using is_t_v = v<is_t_boo<t_in>>;
    template<typename t_in>
    using is_t_v_t = t<is_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_t_stu {
        using type = v<is_t_boo<t_in>>;
    };
}

LGO_EXPORT namespace lgo {
    using is_t_bind = bind<tem<is_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_t_t_boo{ is_t_t_cpt<t_in> };
    template<typename t_in>
    using is_t_t_v = v<is_t_t_boo<t_in>>;
    template<typename t_in>
    using is_t_t_v_t = t<is_t_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_t_t_stu {
        using type = is_t_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_t_t_bind = bind<tem<is_t_t_stu>>;
}

#endif