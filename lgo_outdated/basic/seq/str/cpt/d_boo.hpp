#ifndef LGO_BASIC_SEQ_STR_CPT_D_BOO
#define LGO_BASIC_SEQ_STR_CPT_D_BOO

#include "d_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strd_boo{ is_strd_cpt<t_in> };
    template<typename t_in>
    using is_strd_v = v<is_strd_boo<t_in>>;
    template<typename t_in>
    using is_strd_v_t = t<is_strd_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strd_stu {
        using type = is_strd_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strd_bind = bind<tem<is_strd_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strd_t_boo{ is_strd_t_cpt<t_in> };
    template<typename t_in>
    using is_strd_t_v = v<is_strd_t_boo<t_in>>;
    template<typename t_in>
    using is_strd_t_v_t = t<is_strd_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strd_t_stu {
        using type = is_strd_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strd_t_bind = bind<tem<is_strd_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strd_v_boo{ is_strd_v_cpt<t_in> };
    template<typename t_in>
    using is_strd_v_v = v<is_strd_v_boo<t_in>>;
    template<typename t_in>
    using is_strd_v_v_t = t<is_strd_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strd_v_stu {
        using type = is_strd_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strd_v_bind = bind<tem<is_strd_v_stu>>;
}

#endif