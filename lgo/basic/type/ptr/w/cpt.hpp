#ifndef LGO_BASIC_TYPE_PTR_W_CPT
#define LGO_BASIC_TYPE_PTR_W_CPT

#include "decl.hpp"

#include "../../fun/cast_impl.hpp"
#include "../../boo/.hpp"
#include "../../tem/.hpp"

namespace lgo {
    template<typename>
    struct is_ptrw_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_ptrw_stu<ptrw<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_ptrw_bind = bind<tem<is_ptrw_stu>>;
    template<typename t_in>
    using is_ptrw_v = is_ptrw_bind::type<t_in>;
    template<typename t_in>
    using is_ptrw_v_t = t<is_ptrw_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_ptrw_boo{ is_ptrw_v<t_in>::val };
    template<typename t_in>
    concept is_ptrw_cpt = is_ptrw_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_ptrw_t_cpt = is_t_cpt<t_in> && is_ptrw_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_ptrw_t_boo{ is_ptrw_t_cpt<t_in> };
    template<typename t_in>
    using is_ptrw_t_v = v<is_ptrw_t_boo<t_in>>;
    template<typename t_in>
    using is_ptrw_t_v_t = t<is_ptrw_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ptrw_t_stu {
        using type = is_ptrw_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ptrw_t_bind = bind<tem<is_ptrw_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_ptrw_v_cpt = is_v_cpt<t_in> && is_ptrw_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_ptrw_v_boo{ is_ptrw_v_cpt<t_in> };
    template<typename t_in>
    using is_ptrw_v_v = v<is_ptrw_v_boo<t_in>>;
    template<typename t_in>
    using is_ptrw_v_v_t = t<is_ptrw_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ptrw_v_stu {
        using type = is_ptrw_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ptrw_v_bind = bind<tem<is_ptrw_v_stu>>;
}

#endif