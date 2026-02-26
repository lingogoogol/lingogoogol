#ifndef LGO_BASIC_TYPE_INT_U_BOO
#define LGO_BASIC_TYPE_INT_U_BOO

#include "cpt.hpp"

#include "../../boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_intu_boo{ is_intu_cpt<t_in> };
    template<typename t_in>
    using is_intu_v = v<is_intu_boo<t_in>>;
    template<typename t_in>
    using is_intu_v_t = t<is_intu_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_intu_stu {
        using type = is_intu_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_intu_bind = bind<tem<is_intu_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_intu_t_boo{ is_intu_t_cpt<t_in> };
    template<typename t_in>
    using is_intu_t_v = v<is_intu_t_boo<t_in>>;
    template<typename t_in>
    using is_intu_t_v_t = t<is_intu_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_intu_t_stu {
        using type = is_intu_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_intu_t_bind = bind<tem<is_intu_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_intu_v_boo{ is_intu_v_cpt<t_in> };
    template<typename t_in>
    using is_intu_v_v = v<is_intu_v_boo<t_in>>;
    template<typename t_in>
    using is_intu_v_v_t = t<is_intu_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_intu_v_stu {
        using type = is_intu_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_intu_v_bind = bind<tem<is_intu_v_stu>>;
}

#endif