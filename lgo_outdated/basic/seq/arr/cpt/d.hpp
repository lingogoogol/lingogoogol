#ifndef LGO_BASIC_SEQ_ARR_CPT_D
#define LGO_BASIC_SEQ_ARR_CPT_D

#include "../md/cpt.hpp"
#include "../subd/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrd_cpt = is_arrmd_cpt<t_in> || is_arrsubd_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_arrd_boo{ is_arrd_cpt<t_in> };
    template<typename t_in>
    using is_arrd_v = v<is_arrd_boo<t_in>>;
    template<typename t_in>
    using is_arrd_v_t = t<is_arrd_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrd_stu {
        using type = is_arrd_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrd_bind = bind<tem<is_arrd_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrd_t_cpt = is_t_cpt<t_in> && is_arrd_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrd_t_boo{ is_arrd_t_cpt<t_in> };
    template<typename t_in>
    using is_arrd_t_v = v<is_arrd_t_boo<t_in>>;
    template<typename t_in>
    using is_arrd_t_v_t = t<is_arrd_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrd_t_stu {
        using type = is_arrd_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrd_t_bind = bind<tem<is_arrd_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrd_v_cpt = is_v_cpt<t_in> && is_arrd_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrd_v_boo{ is_arrd_v_cpt<t_in> };
    template<typename t_in>
    using is_arrd_v_v = v<is_arrd_v_boo<t_in>>;
    template<typename t_in>
    using is_arrd_v_v_t = t<is_arrd_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrd_v_stu {
        using type = is_arrd_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrd_v_bind = bind<tem<is_arrd_v_stu>>;
}

#endif