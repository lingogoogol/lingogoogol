#ifndef LGO_BASIC_SEQ_ARR_MD_CPT
#define LGO_BASIC_SEQ_ARR_MD_CPT

#include "decl.hpp"

#include "../../../type/boo/.hpp"

namespace lgo {
    template<typename>
    struct is_arrmd_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_arrmd_stu<arrmd<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrmd_bind = bind<tem<is_arrmd_stu>>;
    template<typename t_in>
    using is_arrmd_v = is_arrmd_bind::type<t_in>;
    template<typename t_in>
    using is_arrmd_v_t = t<is_arrmd_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_arrmd_boo{ is_arrmd_v<t_in>::val };
    template<typename t_in>
    concept is_arrmd_cpt = is_arrmd_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrmd_t_cpt = is_t_cpt<t_in> && is_arrmd_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrmd_t_boo{ is_arrmd_t_cpt<t_in> };
    template<typename t_in>
    using is_arrmd_t_v = v<is_arrmd_t_boo<t_in>>;
    template<typename t_in>
    using is_arrmd_t_v_t = t<is_arrmd_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrmd_t_stu {
        using type = is_arrmd_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrmd_t_bind = bind<tem<is_arrmd_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrmd_v_cpt = is_v_cpt<t_in> && is_arrmd_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrmd_v_boo{ is_arrmd_v_cpt<t_in> };
    template<typename t_in>
    using is_arrmd_v_v = v<is_arrmd_v_boo<t_in>>;
    template<typename t_in>
    using is_arrmd_v_v_t = t<is_arrmd_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrmd_v_stu {
        using type = is_arrmd_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrmd_v_bind = bind<tem<is_arrmd_v_stu>>;
}

#endif