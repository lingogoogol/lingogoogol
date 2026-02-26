#ifndef LGO_BASIC_TYPE_INT_N_BOO
#define LGO_BASIC_TYPE_INT_N_BOO

#include "cpt.hpp"

#include "../../boo/.hpp"
#include "../../tem/bind/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_intn_boo{ is_intn_cpt<t_in> };
    template<typename t_in>
    using is_intn_v = v<is_intn_boo<t_in>>;
    template<typename t_in>
    using is_intn_v_t = t<is_intn_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_intn_stu {
        using type = is_intn_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_intn_bind = bind<tem<is_intn_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_intn_t_boo{ is_intn_t_cpt<t_in> };
    template<typename t_in>
    using is_intn_t_v = v<is_intn_t_boo<t_in>>;
    template<typename t_in>
    using is_intn_t_v_t = t<is_intn_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_intn_t_stu {
        using type = is_intn_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_intn_t_bind = bind<tem<is_intn_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_intn_v_boo{ is_intn_v_cpt<t_in> };
    template<typename t_in>
    using is_intn_v_v = v<is_intn_v_boo<t_in>>;
    template<typename t_in>
    using is_intn_v_v_t = t<is_intn_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_intn_v_stu {
        using type = is_intn_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_intn_v_bind = bind<tem<is_intn_v_stu>>;
}

#endif