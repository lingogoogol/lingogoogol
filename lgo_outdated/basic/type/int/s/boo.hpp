#ifndef LGO_BASIC_TYPE_INT_S_BOO
#define LGO_BASIC_TYPE_INT_S_BOO

#include "cpt.hpp"

#include "../../boo/.hpp"
#include "../../tem/bind/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_ints_boo{ is_ints_cpt<t_in> };
    template<typename t_in>
    using is_ints_v = v<is_ints_boo<t_in>>;
    template<typename t_in>
    using is_ints_v_t = t<is_ints_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ints_stu {
        using type = is_ints_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ints_bind = bind<tem<is_ints_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_ints_t_boo{ is_ints_t_cpt<t_in> };
    template<typename t_in>
    using is_ints_t_v = v<is_ints_t_boo<t_in>>;
    template<typename t_in>
    using is_ints_t_v_t = t<is_ints_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ints_t_stu {
        using type = is_ints_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ints_t_bind = bind<tem<is_ints_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_ints_v_boo{ is_ints_v_cpt<t_in> };
    template<typename t_in>
    using is_ints_v_v = v<is_ints_v_boo<t_in>>;
    template<typename t_in>
    using is_ints_v_v_t = t<is_ints_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ints_v_stu {
        using type = is_ints_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ints_v_bind = bind<tem<is_ints_v_stu>>;
}

#endif