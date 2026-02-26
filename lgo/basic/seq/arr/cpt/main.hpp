#ifndef LGO_BASIC_SEQ_ARR_CPT_MAIN
#define LGO_BASIC_SEQ_ARR_CPT_MAIN

#include "m.hpp"
#include "sub.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arr_cpt = is_arrm_cpt<t_in> || is_arrsub_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_arr_boo{ is_arr_cpt<t_in> };
    template<typename t_in>
    using is_arr_v = v<is_arr_boo<t_in>>;
    template<typename t_in>
    using is_arr_v_t = t<is_arr_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arr_stu {
        using type = is_arr_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arr_bind = bind<tem<is_arr_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arr_t_cpt = is_t_cpt<t_in> && is_arr_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arr_t_boo{ is_arr_t_cpt<t_in> };
    template<typename t_in>
    using is_arr_t_v = v<is_arr_t_boo<t_in>>;
    template<typename t_in>
    using is_arr_t_v_t = t<is_arr_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arr_t_stu {
        using type = is_arr_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arr_t_bind = bind<tem<is_arr_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arr_v_cpt = is_v_cpt<t_in> && is_arr_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arr_v_boo{ is_arr_v_cpt<t_in> };
    template<typename t_in>
    using is_arr_v_v = v<is_arr_v_boo<t_in>>;
    template<typename t_in>
    using is_arr_v_v_t = t<is_arr_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arr_v_stu {
        using type = is_arr_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arr_v_bind = bind<tem<is_arr_v_stu>>;
}

#endif