#ifndef LGO_MATH_NUM_Z_S_CPT
#define LGO_MATH_NUM_Z_S_CPT

#include "interface_decl.hpp"
#include "base_decl.hpp"

namespace lgo {
    template<typename>
    struct is_numzs8_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_numzs8_stu<numzs8<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs8_bind = bind<tem<is_numzs8_stu>>;
    template<typename t_in>
    using is_numzs8_v = is_numzs8_bind::type<t_in>;
    template<typename t_in>
    using is_numzs8_v_t = t<is_numzs8_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_numzs8_boo{ is_numzs8_v<t_in>::val };
	template<typename t_in>
	concept is_numzs8_cpt = is_numzs8_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzs8_t_cpt = is_t_cpt<t_in> && is_numzs8_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzs8_t_boo{ is_numzs8_t_cpt<t_in> };
    template<typename t_in>
    using is_numzs8_t_v = v<is_numzs8_t_boo<t_in>>;
    template<typename t_in>
    using is_numzs8_t_v_t = t<is_numzs8_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzs8_t_stu {
        using type = is_numzs8_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs8_t_bind = bind<tem<is_numzs8_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzs8_v_cpt = is_v_cpt<t_in> && is_numzs8_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzs8_v_boo{ is_numzs8_v_cpt<t_in> };
    template<typename t_in>
    using is_numzs8_v_v = v<is_numzs8_v_boo<t_in>>;
    template<typename t_in>
    using is_numzs8_v_v_t = t<is_numzs8_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzs8_v_stu {
        using type = is_numzs8_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs8_v_bind = bind<tem<is_numzs8_v_stu>>;
}

namespace lgo {
    template<typename>
    struct is_numzs16_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_numzs16_stu<numzs16<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs16_bind = bind<tem<is_numzs16_stu>>;
    template<typename t_in>
    using is_numzs16_v = is_numzs16_bind::type<t_in>;
    template<typename t_in>
    using is_numzs16_v_t = t<is_numzs16_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_numzs16_boo{ is_numzs16_v<t_in>::val };
	template<typename t_in>
	concept is_numzs16_cpt = is_numzs16_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzs16_t_cpt = is_t_cpt<t_in> && is_numzs16_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzs16_t_boo{ is_numzs16_t_cpt<t_in> };
    template<typename t_in>
    using is_numzs16_t_v = v<is_numzs16_t_boo<t_in>>;
    template<typename t_in>
    using is_numzs16_t_v_t = t<is_numzs16_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzs16_t_stu {
        using type = is_numzs16_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs16_t_bind = bind<tem<is_numzs16_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzs16_v_cpt = is_v_cpt<t_in> && is_numzs16_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzs16_v_boo{ is_numzs16_v_cpt<t_in> };
    template<typename t_in>
    using is_numzs16_v_v = v<is_numzs16_v_boo<t_in>>;
    template<typename t_in>
    using is_numzs16_v_v_t = t<is_numzs16_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzs16_v_stu {
        using type = is_numzs16_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs16_v_bind = bind<tem<is_numzs16_v_stu>>;
}

namespace lgo {
    template<typename>
    struct is_numzs32_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_numzs32_stu<numzs32<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs32_bind = bind<tem<is_numzs32_stu>>;
    template<typename t_in>
    using is_numzs32_v = is_numzs32_bind::type<t_in>;
    template<typename t_in>
    using is_numzs32_v_t = t<is_numzs32_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_numzs32_boo{ is_numzs32_v<t_in>::val };
	template<typename t_in>
	concept is_numzs32_cpt = is_numzs32_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzs32_t_cpt = is_t_cpt<t_in> && is_numzs32_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzs32_t_boo{ is_numzs32_t_cpt<t_in> };
    template<typename t_in>
    using is_numzs32_t_v = v<is_numzs32_t_boo<t_in>>;
    template<typename t_in>
    using is_numzs32_t_v_t = t<is_numzs32_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzs32_t_stu {
        using type = is_numzs32_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs32_t_bind = bind<tem<is_numzs32_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzs32_v_cpt = is_v_cpt<t_in> && is_numzs32_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzs32_v_boo{ is_numzs32_v_cpt<t_in> };
    template<typename t_in>
    using is_numzs32_v_v = v<is_numzs32_v_boo<t_in>>;
    template<typename t_in>
    using is_numzs32_v_v_t = t<is_numzs32_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzs32_v_stu {
        using type = is_numzs32_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs32_v_bind = bind<tem<is_numzs32_v_stu>>;
}

namespace lgo {
    template<typename>
    struct is_numzs64_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_numzs64_stu<numzs64<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs64_bind = bind<tem<is_numzs64_stu>>;
    template<typename t_in>
    using is_numzs64_v = is_numzs64_bind::type<t_in>;
    template<typename t_in>
    using is_numzs64_v_t = t<is_numzs64_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_numzs64_boo{ is_numzs64_v<t_in>::val };
	template<typename t_in>
	concept is_numzs64_cpt = is_numzs64_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzs64_t_cpt = is_t_cpt<t_in> && is_numzs64_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzs64_t_boo{ is_numzs64_t_cpt<t_in> };
    template<typename t_in>
    using is_numzs64_t_v = v<is_numzs64_t_boo<t_in>>;
    template<typename t_in>
    using is_numzs64_t_v_t = t<is_numzs64_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzs64_t_stu {
        using type = is_numzs64_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs64_t_bind = bind<tem<is_numzs64_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzs64_v_cpt = is_v_cpt<t_in> && is_numzs64_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzs64_v_boo{ is_numzs64_v_cpt<t_in> };
    template<typename t_in>
    using is_numzs64_v_v = v<is_numzs64_v_boo<t_in>>;
    template<typename t_in>
    using is_numzs64_v_v_t = t<is_numzs64_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzs64_v_stu {
        using type = is_numzs64_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs64_v_bind = bind<tem<is_numzs64_v_stu>>;
}

namespace lgo {
    using is_numzsc_stu = is_numzs64_stu;
}

LGO_EXPORT namespace lgo {
    using is_numzsc_bind = bind<tem<is_numzsc_stu>>;
    template<typename t_in>
    using is_numzsc_v = is_numzsc_bind::type<t_in>;
    template<typename t_in>
    using is_numzsc_v_t = t<is_numzsc_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_numzsc_boo{ is_numzsc_v<t_in>::val };
	template<typename t_in>
	concept is_numzsc_cpt = is_numzsc_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzsc_t_cpt = is_t_cpt<t_in> && is_numzsc_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzsc_t_boo{ is_numzsc_t_cpt<t_in> };
    template<typename t_in>
    using is_numzsc_t_v = v<is_numzsc_t_boo<t_in>>;
    template<typename t_in>
    using is_numzsc_t_v_t = t<is_numzsc_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzsc_t_stu {
        using type = is_numzsc_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzsc_t_bind = bind<tem<is_numzsc_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzsc_v_cpt = is_v_cpt<t_in> && is_numzsc_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzsc_v_boo{ is_numzsc_v_cpt<t_in> };
    template<typename t_in>
    using is_numzsc_v_v = v<is_numzsc_v_boo<t_in>>;
    template<typename t_in>
    using is_numzsc_v_v_t = t<is_numzsc_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzsc_v_stu {
        using type = is_numzsc_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzsc_v_bind = bind<tem<is_numzsc_v_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzs_cpt = is_numzs8_cpt<t_in> || is_numzs16_cpt<t_in>
    || is_numzs32_cpt<t_in> || is_numzs64_cpt<t_in> || is_numzsc_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_numzs_boo{ is_numzs_cpt<t_in> };
    template<typename t_in>
    using is_numzs_v = v<is_numzs_boo<t_in>>;
    template<typename t_in>
    using is_numzs_v_t = t<is_numzs_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzs_stu {
        using type = is_numzs_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs_bind = bind<tem<is_numzs_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzs_t_cpt = is_t_cpt<t_in> && is_numzs_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzs_t_boo{ is_numzs_t_cpt<t_in> };
    template<typename t_in>
    using is_numzs_t_v = v<is_numzs_t_boo<t_in>>;
    template<typename t_in>
    using is_numzs_t_v_t = t<is_numzs_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzs_t_stu {
        using type = is_numzs_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs_t_bind = bind<tem<is_numzs_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzs_v_cpt = is_v_cpt<t_in> && is_numzs_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzs_v_boo{ is_numzs_v_cpt<t_in> };
    template<typename t_in>
    using is_numzs_v_v = v<is_numzs_v_boo<t_in>>;
    template<typename t_in>
    using is_numzs_v_v_t = t<is_numzs_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzs_v_stu {
        using type = is_numzs_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzs_v_bind = bind<tem<is_numzs_v_stu>>;
}

namespace lgo {
    template<typename>
    struct is_numzs_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<typename... t_in>
    struct is_numzs_cpt_stu<numzs<t_in...>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

namespace lgo {
    template<typename t_in>
    concept is_numzs_impl_cpt = is_numzs_cpt<t_in> || is_numzs_cpt_stu<t_in>::val;
}

#endif