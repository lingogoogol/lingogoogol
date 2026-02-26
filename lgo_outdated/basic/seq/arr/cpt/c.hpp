#ifndef LGO_BASIC_SEQ_ARR_CPT_C
#define LGO_BASIC_SEQ_ARR_CPT_C

#include "s.hpp"

#include "../mc/main/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrc_cpt = is_arrs_cpt<t_in> || is_arrmc_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_arrc_boo{ is_arrc_cpt<t_in> };
    template<typename t_in>
    using is_arrc_v = v<is_arrc_boo<t_in>>;
    template<typename t_in>
    using is_arrc_v_t = t<is_arrc_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrc_stu {
        using type = is_arrc_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrc_bind = bind<tem<is_arrc_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrc_t_cpt = is_t_cpt<t_in> && is_arrc_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrc_t_boo{ is_arrc_t_cpt<t_in> };
    template<typename t_in>
    using is_arrc_t_v = v<is_arrc_t_boo<t_in>>;
    template<typename t_in>
    using is_arrc_t_v_t = t<is_arrc_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrc_t_stu {
        using type = is_arrc_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrc_t_bind = bind<tem<is_arrc_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrc_v_cpt = is_v_cpt<t_in> && is_arrc_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrc_v_boo{ is_arrc_v_cpt<t_in> };
    template<typename t_in>
    using is_arrc_v_v = v<is_arrc_v_boo<t_in>>;
    template<typename t_in>
    using is_arrc_v_v_t = t<is_arrc_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrc_v_stu {
        using type = is_arrc_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrc_v_bind = bind<tem<is_arrc_v_stu>>;
}

#endif