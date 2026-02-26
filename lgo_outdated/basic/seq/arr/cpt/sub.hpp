#ifndef LGO_BASIC_SEQ_ARR_CPT_SUB
#define LGO_BASIC_SEQ_ARR_CPT_SUB

#include "../subs/cpt.hpp"
#include "../subd/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrsub_cpt = is_arrsubs_cpt<t_in> || is_arrsubd_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_arrsub_boo{ is_arrsub_cpt<t_in> };
    template<typename t_in>
    using is_arrsub_v = v<is_arrsub_boo<t_in>>;
    template<typename t_in>
    using is_arrsub_v_t = t<is_arrsub_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrsub_stu {
        using type = is_arrsub_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrsub_bind = bind<tem<is_arrsub_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrsub_t_cpt = is_t_cpt<t_in> && is_arrsub_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrsub_t_boo{ is_arrsub_t_cpt<t_in> };
    template<typename t_in>
    using is_arrsub_t_v = v<is_arrsub_t_boo<t_in>>;
    template<typename t_in>
    using is_arrsub_t_v_t = t<is_arrsub_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrsub_t_stu {
        using type = is_arrsub_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrsub_t_bind = bind<tem<is_arrsub_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrsub_v_cpt = is_v_cpt<t_in> && is_arrsub_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrsub_v_boo{ is_arrsub_v_cpt<t_in> };
    template<typename t_in>
    using is_arrsub_v_v = v<is_arrsub_v_boo<t_in>>;
    template<typename t_in>
    using is_arrsub_v_v_t = t<is_arrsub_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrsub_v_stu {
        using type = is_arrsub_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrsub_v_bind = bind<tem<is_arrsub_v_stu>>;
}

#endif