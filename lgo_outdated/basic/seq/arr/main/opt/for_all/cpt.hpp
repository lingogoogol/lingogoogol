#ifndef LGO_BASIC_SEQ_ARR_MAIN_OPT_FOR_ALL_CPT
#define LGO_BASIC_SEQ_ARR_MAIN_OPT_FOR_ALL_CPT

#include "decl.hpp"

namespace lgo {
    template<typename>
    struct is_arr_for_all_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_arr_for_all_stu<arr_for_all<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_arr_for_all_bind = bind<tem<is_arr_for_all_stu>>;
    template<typename t_in>
    using is_arr_for_all_v = is_arr_for_all_bind::type<t_in>;
    template<typename t_in>
    using is_arr_for_all_v_t = t<is_arr_for_all_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_arr_for_all_boo{ is_arr_for_all_v<t_in>::val };
    template<typename t_in>
    concept is_arr_for_all_cpt = is_arr_for_all_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arr_for_all_t_cpt = is_t_cpt<t_in> && is_arr_for_all_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arr_for_all_t_boo{ is_arr_for_all_t_cpt<t_in> };
    template<typename t_in>
    using is_arr_for_all_t_v = v<is_arr_for_all_t_boo<t_in>>;
    template<typename t_in>
    using is_arr_for_all_t_v_t = t<is_arr_for_all_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arr_for_all_t_stu {
        using type = is_arr_for_all_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arr_for_all_t_bind = bind<tem<is_arr_for_all_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arr_for_all_v_cpt = is_v_cpt<t_in> && is_arr_for_all_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arr_for_all_v_boo{ is_arr_for_all_v_cpt<t_in> };
    template<typename t_in>
    using is_arr_for_all_v_v = v<is_arr_for_all_v_boo<t_in>>;
    template<typename t_in>
    using is_arr_for_all_v_v_t = t<is_arr_for_all_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arr_for_all_v_stu {
        using type = is_arr_for_all_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arr_for_all_v_bind = bind<tem<is_arr_for_all_v_stu>>;
}

#endif