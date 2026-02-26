#ifndef LGO_BASIC_TYPE_PTR_M_CPT
#define LGO_BASIC_TYPE_PTR_M_CPT

#include "decl.hpp"

#include "../../fun/cast_impl.hpp"
#include "../../boo/.hpp"
#include "../../tem/.hpp"

namespace lgo {
    template<typename>
    struct is_ptrm_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_ptrm_stu<ptrm<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_ptrm_bind = bind<tem<is_ptrm_stu>>;
    template<typename t_in>
    using is_ptrm_v = is_ptrm_bind::type<t_in>;
    template<typename t_in>
    using is_ptrm_v_t = t<is_ptrm_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_ptrm_boo{ is_ptrm_v<t_in>::val };
    template<typename t_in>
    concept is_ptrm_cpt = is_ptrm_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_ptrm_t_cpt = is_t_cpt<t_in> && is_ptrm_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_ptrm_t_boo{ is_ptrm_t_cpt<t_in> };
    template<typename t_in>
    using is_ptrm_t_v = v<is_ptrm_t_boo<t_in>>;
    template<typename t_in>
    using is_ptrm_t_v_t = t<is_ptrm_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ptrm_t_stu {
        using type = is_ptrm_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ptrm_t_bind = bind<tem<is_ptrm_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_ptrm_v_cpt = is_v_cpt<t_in> && is_ptrm_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_ptrm_v_boo{ is_ptrm_v_cpt<t_in> };
    template<typename t_in>
    using is_ptrm_v_v = v<is_ptrm_v_boo<t_in>>;
    template<typename t_in>
    using is_ptrm_v_v_t = t<is_ptrm_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ptrm_v_stu {
        using type = is_ptrm_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ptrm_v_bind = bind<tem<is_ptrm_v_stu>>;
}

#endif