#ifndef LGO_BASIC_SEQ_ARR_SUBS_CPT
#define LGO_BASIC_SEQ_ARR_SUBS_CPT

#include "decl.hpp"

#include "../../../type/boo/.hpp"
#include "../../../type/fun/cast_impl.hpp"

namespace lgo {
    template<typename>
    struct is_arrsubs_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_arrsubs_stu<arrsubs<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrsubs_bind = bind<tem<is_arrsubs_stu>>;
    template<typename t_in>
    using is_arrsubs_v = is_arrsubs_bind::type<t_in>;
    template<typename t_in>
    using is_arrsubs_v_t = t<is_arrsubs_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_arrsubs_boo{ is_arrsubs_v<t_in>::val };
    template<typename t_in>
    concept is_arrsubs_cpt = is_arrsubs_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrsubs_t_cpt = is_t_cpt<t_in> && is_arrsubs_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrsubs_t_boo{ is_arrsubs_t_cpt<t_in> };
    template<typename t_in>
    using is_arrsubs_t_v = v<is_arrsubs_t_boo<t_in>>;
    template<typename t_in>
    using is_arrsubs_t_v_t = t<is_arrsubs_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrsubs_t_stu {
        using type = is_arrsubs_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrsubs_t_bind = bind<tem<is_arrsubs_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrsubs_v_cpt = is_v_cpt<t_in> && is_arrsubs_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrsubs_v_boo{ is_arrsubs_v_cpt<t_in> };
    template<typename t_in>
    using is_arrsubs_v_v = v<is_arrsubs_v_boo<t_in>>;
    template<typename t_in>
    using is_arrsubs_v_v_t = t<is_arrsubs_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrsubs_v_stu {
        using type = is_arrsubs_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrsubs_v_bind = bind<tem<is_arrsubs_v_stu>>;
}

#endif