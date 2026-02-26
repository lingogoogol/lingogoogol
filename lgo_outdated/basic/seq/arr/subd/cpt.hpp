#ifndef LGO_BASIC_SEQ_ARR_SUBD_CPT
#define LGO_BASIC_SEQ_ARR_SUBD_CPT

#include "decl.hpp"

#include "../../../type/boo/.hpp"

namespace lgo {
    template<typename>
    struct is_arrsubd_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_arrsubd_stu<arrsubd<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrsubd_bind = bind<tem<is_arrsubd_stu>>;
    template<typename t_in>
    using is_arrsubd_v = is_arrsubd_bind::type<t_in>;
    template<typename t_in>
    using is_arrsubd_v_t = t<is_arrsubd_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_arrsubd_boo{ is_arrsubd_v<t_in>::val };
    template<typename t_in>
    concept is_arrsubd_cpt = is_arrsubd_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrsubd_t_cpt = is_t_cpt<t_in> && is_arrsubd_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrsubd_t_boo{ is_arrsubd_t_cpt<t_in> };
    template<typename t_in>
    using is_arrsubd_t_v = v<is_arrsubd_t_boo<t_in>>;
    template<typename t_in>
    using is_arrsubd_t_v_t = t<is_arrsubd_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrsubd_t_stu {
        using type = is_arrsubd_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrsubd_t_bind = bind<tem<is_arrsubd_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrsubd_v_cpt = is_v_cpt<t_in> && is_arrsubd_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrsubd_v_boo{ is_arrsubd_v_cpt<t_in> };
    template<typename t_in>
    using is_arrsubd_v_v = v<is_arrsubd_v_boo<t_in>>;
    template<typename t_in>
    using is_arrsubd_v_v_t = t<is_arrsubd_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrsubd_v_stu {
        using type = is_arrsubd_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrsubd_v_bind = bind<tem<is_arrsubd_v_stu>>;
}

#endif