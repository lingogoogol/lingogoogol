#ifndef LGO_BASIC_SEQ_ARR_MS_CPT
#define LGO_BASIC_SEQ_ARR_MS_CPT

#include "decl.hpp"

#include "../../../type/boo/.hpp"

namespace lgo {
    template<typename>
    struct is_arrms_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_arrms_stu<arrms<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrms_bind = bind<tem<is_arrms_stu>>;
    template<typename t_in>
    using is_arrms_v = is_arrms_bind::type<t_in>;
    template<typename t_in>
    using is_arrms_v_t = t<is_arrms_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_arrms_boo{ is_arrms_v<t_in>::val };
    template<typename t_in>
    concept is_arrms_cpt = is_arrms_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrms_t_cpt = is_t_cpt<t_in> && is_arrms_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrms_t_boo{ is_arrms_t_cpt<t_in> };
    template<typename t_in>
    using is_arrms_t_v = v<is_arrms_t_boo<t_in>>;
    template<typename t_in>
    using is_arrms_t_v_t = t<is_arrms_t_v<t_in>>;
}


namespace lgo {
    template<typename t_in>
    struct is_arrms_t_stu {
        using type = is_arrms_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrms_t_bind = bind<tem<is_arrms_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrms_v_cpt = is_v_cpt<t_in> && is_arrms_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrms_v_boo{ is_arrms_v_cpt<t_in> };
    template<typename t_in>
    using is_arrms_v_v = v<is_arrms_v_boo<t_in>>;
    template<typename t_in>
    using is_arrms_v_v_t = t<is_arrms_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrms_v_stu {
        using type = is_arrms_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrms_v_bind = bind<tem<is_arrms_v_stu>>;
}

#endif