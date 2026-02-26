#ifndef LGO_BASIC_SEQ_ARR_CPT_S
#define LGO_BASIC_SEQ_ARR_CPT_S

#include "d.hpp"

#include "../ms/cpt.hpp"
#include "../subs/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrs_cpt = is_arrd_cpt<t_in> || is_arrms_cpt<t_in> || is_arrsubs_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_arrs_boo{ is_arrs_cpt<t_in> };
    template<typename t_in>
    using is_arrs_v = v<is_arrs_boo<t_in>>;
    template<typename t_in>
    using is_arrs_v_t = t<is_arrs_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrs_stu {
        using type = is_arrs_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrs_bind = bind<tem<is_arrs_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrs_t_cpt = is_t_cpt<t_in> && is_arrs_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrs_t_boo{ is_arrs_t_cpt<t_in> };
    template<typename t_in>
    using is_arrs_t_v = v<is_arrs_t_boo<t_in>>;
    template<typename t_in>
    using is_arrs_t_v_t = t<is_arrs_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrs_t_stu {
        using type = is_arrs_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrs_t_bind = bind<tem<is_arrs_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrs_v_cpt = is_v_cpt<t_in> && is_arrs_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrs_v_boo{ is_arrs_v_cpt<t_in> };
    template<typename t_in>
    using is_arrs_v_v = v<is_arrs_v_boo<t_in>>;
    template<typename t_in>
    using is_arrs_v_v_t = t<is_arrs_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrs_v_stu {
        using type = is_arrs_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrs_v_bind = bind<tem<is_arrs_v_stu>>;
}

#endif