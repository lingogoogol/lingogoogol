#ifndef LGO_BASIC_FUN_CPT
#define LGO_BASIC_FUN_CPT

#include "decl.hpp"

#include "../type/boo/.hpp"
#include "../type/fun/cast_impl.hpp"
#include "../type/tem/t/.hpp"

namespace lgo {
    template<typename>
    struct is_fun_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_fun_stu<fun<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_fun_bind = bind<tem<is_fun_stu>>;
    template<typename t_in>
    using is_fun_v = is_fun_bind::type<t_in>;
    template<typename t_in>
    using is_fun_v_t = t<is_fun_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_fun_boo{ is_fun_v<t_in>::val };
    template<typename t_in>
    concept is_fun_cpt = is_fun_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_fun_t_cpt = is_t_cpt<t_in> && is_fun_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_fun_t_boo{ is_fun_t_cpt<t_in> };
    template<typename t_in>
    using is_fun_t_v = v<is_fun_t_boo<t_in>>;
    template<typename t_in>
    using is_fun_t_v_t = t<is_fun_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_fun_t_stu {
        using type = is_fun_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_fun_t_bind = bind<tem<is_fun_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_fun_v_cpt = is_v_cpt<t_in> && is_fun_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_fun_v_boo{ is_fun_v_cpt<t_in> };
    template<typename t_in>
    using is_fun_v_v = v<is_fun_v_boo<t_in>>;
    template<typename t_in>
    using is_fun_v_v_t = t<is_fun_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_fun_v_stu {
        using type = is_fun_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_fun_v_bind = bind<tem<is_fun_v_stu>>;
}

#endif