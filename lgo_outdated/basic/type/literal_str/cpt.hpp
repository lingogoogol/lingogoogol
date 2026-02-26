#ifndef LGO_BASIC_TYPE_LITERAL_DECL_CPT
#define LGO_BASIC_TYPE_LITERAL_DECL_CPT

#include "decl.hpp"

#include "../boo/.hpp"
#include "../tem/.hpp"
#include "../fun/cast_impl.hpp"

namespace lgo {
    template<typename>
    struct is_literal_str_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_literal_str_stu<literal_str<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_literal_str_bind = bind<tem<is_literal_str_stu>>;
    template<typename t_in>
    using is_literal_str_v = is_literal_str_bind::type<t_in>;
    template<typename t_in>
    using is_literal_str_v_t = t<is_literal_str_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_literal_str_boo{ is_literal_str_v<t_in>::val };
    template<typename t_in>
    concept is_literal_str_cpt = is_literal_str_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_literal_str_t_cpt = is_t_cpt<t_in> && is_literal_str_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_literal_str_t_boo{ is_literal_str_t_cpt<t_in> };
    template<typename t_in>
    using is_literal_str_t_v = v<is_literal_str_t_boo<t_in>>;
    template<typename t_in>
    using is_literal_str_t_v_t = t<is_literal_str_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_literal_str_t_stu {
        using type = is_literal_str_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_literal_str_t_bind = bind<tem<is_literal_str_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_literal_str_v_cpt = is_v_cpt<t_in> && is_literal_str_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_literal_str_v_boo{ is_literal_str_v_cpt<t_in> };
    template<typename t_in>
    using is_literal_str_v_v = v<is_literal_str_v_boo<t_in>>;
    template<typename t_in>
    using is_literal_str_v_v_t = t<is_literal_str_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_literal_str_v_stu {
        using type = is_literal_str_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_literal_str_v_bind = bind<tem<is_literal_str_v_stu>>;
}

#endif