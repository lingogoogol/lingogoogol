#ifndef LGO_BASIC_SEQ_ARR_MC_MAIN_CPT
#define LGO_BASIC_SEQ_ARR_MC_MAIN_CPT

#include "decl.hpp"

#include "../../../../type/boo/impl.hpp"

namespace lgo {
    template<typename>
    struct is_arrmc_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_arrmc_stu<arrmc<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrmc_bind = bind<tem<is_arrmc_stu>>;
    template<typename t_in>
    using is_arrmc_v = is_arrmc_bind::type<t_in>;
    template<typename t_in>
    using is_arrmc_v_t = t<is_arrmc_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_arrmc_boo{ is_arrmc_v<t_in>::val };
    template<typename t_in>
    concept is_arrmc_cpt = is_arrmc_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrmc_t_cpt = is_t_cpt<t_in> && is_arrmc_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrmc_t_boo{ is_arrmc_t_cpt<t_in> };
    template<typename t_in>
    using is_arrmc_t_v = v<is_arrmc_t_boo<t_in>>;
    template<typename t_in>
    using is_arrmc_t_v_t = t<is_arrmc_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrmc_t_stu {
        using type = is_arrmc_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrmc_t_bind = bind<tem<is_arrmc_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arrmc_v_cpt = is_v_cpt<t_in> && is_arrmc_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arrmc_v_boo{ is_arrmc_v_cpt<t_in> };
    template<typename t_in>
    using is_arrmc_v_v = v<is_arrmc_v_boo<t_in>>;
    template<typename t_in>
    using is_arrmc_v_v_t = t<is_arrmc_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arrmc_v_stu {
        using type = is_arrmc_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arrmc_v_bind = bind<tem<is_arrmc_v_stu>>;
}

#endif