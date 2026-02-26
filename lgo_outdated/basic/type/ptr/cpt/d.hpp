#ifndef LGO_BASIC_TYPE_PTR_CPT_D
#define LGO_BASIC_TYPE_PTR_CPT_D

#include "../m/cpt.hpp"
#include "../w/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_ptrd_cpt = is_ptrm_cpt<t_in> || is_ptrw_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_ptrd_boo{ is_ptrd_cpt<t_in> };
    template<typename t_in>
    using is_ptrd_v = v<is_ptrd_boo<t_in>>;
    template<typename t_in>
    using is_ptrd_v_t = t<is_ptrd_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ptrd_stu {
        using type = is_ptrd_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ptrd_bind = bind<tem<is_ptrd_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_ptrd_t_cpt = is_t_cpt<t_in> && is_ptrd_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_ptrd_t_boo{ is_ptrd_t_cpt<t_in> };
    template<typename t_in>
    using is_ptrd_t_v = v<is_ptrd_t_boo<t_in>>;
    template<typename t_in>
    using is_ptrd_t_v_t = t<is_ptrd_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ptrd_t_stu {
        using type = is_ptrd_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ptrd_t_bind = bind<tem<is_ptrd_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_ptrd_v_cpt = is_v_cpt<t_in> && is_ptrd_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_ptrd_v_boo{ is_ptrd_v_cpt<t_in> };
    template<typename t_in>
    using is_ptrd_v_v = v<is_ptrd_v_boo<t_in>>;
    template<typename t_in>
    using is_ptrd_v_v_t = t<is_ptrd_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ptrd_v_stu {
        using type = is_ptrd_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ptrd_v_bind = bind<tem<is_ptrd_v_stu>>;
}

#endif