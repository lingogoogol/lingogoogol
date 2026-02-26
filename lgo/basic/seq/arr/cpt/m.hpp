#ifndef LGO_BASIC_SEQ_ARR_CPT_M
#define LGO_BASIC_SEQ_ARR_CPT_M

#include "../mc/main/cpt.hpp"
#include "../ms/cpt.hpp"
#include "../md/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrm_cpt = is_arrmc_cpt<t_in> || is_arrms_cpt<t_in> || is_arrmd_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_arrm_boo{ is_arrm_cpt<t_in> };
    template<typename t_in>
    using is_arrm_v = v<is_arrm_boo<t_in>>;
    template<typename t_in>
    using is_arrm_v_t = t<is_arrm_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrm_stu {
        using type = is_arrm_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrm_bind = bind<tem<is_arrm_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrm_t_cpt = is_t_cpt<t_in> && is_arrm_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrm_t_boo{ is_arrm_t_cpt<t_in> };
    template<typename t_in>
    using is_arrm_t_v = v<is_arrm_t_boo<t_in>>;
    template<typename t_in>
    using is_arrm_t_v_t = t<is_arrm_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrm_t_stu {
        using type = is_arrm_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrm_t_bind = bind<tem<is_arrm_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrm_v_cpt = is_v_cpt<t_in> && is_arrm_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrm_v_boo{ is_arrm_v_cpt<t_in> };
    template<typename t_in>
    using is_arrm_v_v = v<is_arrm_v_boo<t_in>>;
    template<typename t_in>
    using is_arrm_v_v_t = t<is_arrm_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrm_v_stu {
        using type = is_arrm_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrm_v_bind = bind<tem<is_arrm_v_stu>>;
}

#endif