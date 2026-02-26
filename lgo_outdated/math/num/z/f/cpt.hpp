#ifndef LGO_MATH_NUM_Z_F_CPT
#define LGO_MATH_NUM_Z_F_CPT

#include "interface_decl.hpp"
#include "base_decl.hpp"

namespace lgo {
    template<typename>
    struct is_numzf8_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_numzf8_stu<numzf8<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf8_bind = bind<tem<is_numzf8_stu>>;
    template<typename t_in>
    using is_numzf8_v = is_numzf8_bind::type<t_in>;
    template<typename t_in>
    using is_numzf8_v_t = t<is_numzf8_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_numzf8_boo{ is_numzf8_v<t_in>::val };
	template<typename t_in>
	concept is_numzf8_cpt = is_numzf8_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzf8_t_cpt = is_t_cpt<t_in> && is_numzf8_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzf8_t_boo{ is_numzf8_t_cpt<t_in> };
    template<typename t_in>
    using is_numzf8_t_v = v<is_numzf8_t_boo<t_in>>;
    template<typename t_in>
    using is_numzf8_t_v_t = t<is_numzf8_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzf8_t_stu {
        using type = is_numzf8_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf8_t_bind = bind<tem<is_numzf8_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzf8_v_cpt = is_v_cpt<t_in> && is_numzf8_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzf8_v_boo{ is_numzf8_v_cpt<t_in> };
    template<typename t_in>
    using is_numzf8_v_v = v<is_numzf8_v_boo<t_in>>;
    template<typename t_in>
    using is_numzf8_v_v_t = t<is_numzf8_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzf8_v_stu {
        using type = is_numzf8_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf8_v_bind = bind<tem<is_numzf8_v_stu>>;
}

namespace lgo {
    template<typename>
    struct is_numzf16_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_numzf16_stu<numzf16<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf16_bind = bind<tem<is_numzf16_stu>>;
    template<typename t_in>
    using is_numzf16_v = is_numzf16_bind::type<t_in>;
    template<typename t_in>
    using is_numzf16_v_t = t<is_numzf16_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_numzf16_boo{ is_numzf16_v<t_in>::val };
	template<typename t_in>
	concept is_numzf16_cpt = is_numzf16_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzf16_t_cpt = is_t_cpt<t_in> && is_numzf16_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzf16_t_boo{ is_numzf16_t_cpt<t_in> };
    template<typename t_in>
    using is_numzf16_t_v = v<is_numzf16_t_boo<t_in>>;
    template<typename t_in>
    using is_numzf16_t_v_t = t<is_numzf16_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzf16_t_stu {
        using type = is_numzf16_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf16_t_bind = bind<tem<is_numzf16_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzf16_v_cpt = is_v_cpt<t_in> && is_numzf16_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzf16_v_boo{ is_numzf16_v_cpt<t_in> };
    template<typename t_in>
    using is_numzf16_v_v = v<is_numzf16_v_boo<t_in>>;
    template<typename t_in>
    using is_numzf16_v_v_t = t<is_numzf16_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzf16_v_stu {
        using type = is_numzf16_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf16_v_bind = bind<tem<is_numzf16_v_stu>>;
}

namespace lgo {
    template<typename>
    struct is_numzf32_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_numzf32_stu<numzf32<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf32_bind = bind<tem<is_numzf32_stu>>;
    template<typename t_in>
    using is_numzf32_v = is_numzf32_bind::type<t_in>;
    template<typename t_in>
    using is_numzf32_v_t = t<is_numzf32_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_numzf32_boo{ is_numzf32_v<t_in>::val };
	template<typename t_in>
	concept is_numzf32_cpt = is_numzf32_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzf32_t_cpt = is_t_cpt<t_in> && is_numzf32_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzf32_t_boo{ is_numzf32_t_cpt<t_in> };
    template<typename t_in>
    using is_numzf32_t_v = v<is_numzf32_t_boo<t_in>>;
    template<typename t_in>
    using is_numzf32_t_v_t = t<is_numzf32_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzf32_t_stu {
        using type = is_numzf32_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf32_t_bind = bind<tem<is_numzf32_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzf32_v_cpt = is_v_cpt<t_in> && is_numzf32_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzf32_v_boo{ is_numzf32_v_cpt<t_in> };
    template<typename t_in>
    using is_numzf32_v_v = v<is_numzf32_v_boo<t_in>>;
    template<typename t_in>
    using is_numzf32_v_v_t = t<is_numzf32_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzf32_v_stu {
        using type = is_numzf32_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf32_v_bind = bind<tem<is_numzf32_v_stu>>;
}

namespace lgo {
    template<typename>
    struct is_numzf64_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_numzf64_stu<numzf64<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf64_bind = bind<tem<is_numzf64_stu>>;
    template<typename t_in>
    using is_numzf64_v = is_numzf64_bind::type<t_in>;
    template<typename t_in>
    using is_numzf64_v_t = t<is_numzf64_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_numzf64_boo{ is_numzf64_v<t_in>::val };
	template<typename t_in>
	concept is_numzf64_cpt = is_numzf64_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzf64_t_cpt = is_t_cpt<t_in> && is_numzf64_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzf64_t_boo{ is_numzf64_t_cpt<t_in> };
    template<typename t_in>
    using is_numzf64_t_v = v<is_numzf64_t_boo<t_in>>;
    template<typename t_in>
    using is_numzf64_t_v_t = t<is_numzf64_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzf64_t_stu {
        using type = is_numzf64_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf64_t_bind = bind<tem<is_numzf64_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzf64_v_cpt = is_v_cpt<t_in> && is_numzf64_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzf64_v_boo{ is_numzf64_v_cpt<t_in> };
    template<typename t_in>
    using is_numzf64_v_v = v<is_numzf64_v_boo<t_in>>;
    template<typename t_in>
    using is_numzf64_v_v_t = t<is_numzf64_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzf64_v_stu {
        using type = is_numzf64_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf64_v_bind = bind<tem<is_numzf64_v_stu>>;
}

namespace lgo {
    using is_numzfc_stu = is_numzf64_stu;
}

LGO_EXPORT namespace lgo {
    using is_numzfc_bind = bind<tem<is_numzfc_stu>>;
    template<typename t_in>
    using is_numzfc_v = is_numzfc_bind::type<t_in>;
    template<typename t_in>
    using is_numzfc_v_t = t<is_numzfc_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_numzfc_boo{ is_numzfc_v<t_in>::val };
	template<typename t_in>
	concept is_numzfc_cpt = is_numzfc_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzfc_t_cpt = is_t_cpt<t_in> && is_numzfc_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzfc_t_boo{ is_numzfc_t_cpt<t_in> };
    template<typename t_in>
    using is_numzfc_t_v = v<is_numzfc_t_boo<t_in>>;
    template<typename t_in>
    using is_numzfc_t_v_t = t<is_numzfc_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzfc_t_stu {
        using type = is_numzfc_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzfc_t_bind = bind<tem<is_numzfc_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzfc_v_cpt = is_v_cpt<t_in> && is_numzfc_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzfc_v_boo{ is_numzfc_v_cpt<t_in> };
    template<typename t_in>
    using is_numzfc_v_v = v<is_numzfc_v_boo<t_in>>;
    template<typename t_in>
    using is_numzfc_v_v_t = t<is_numzfc_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzfc_v_stu {
        using type = is_numzfc_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzfc_v_bind = bind<tem<is_numzfc_v_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzf_cpt = is_numzf8_cpt<t_in> || is_numzf16_cpt<t_in>
    || is_numzf32_cpt<t_in> || is_numzf64_cpt<t_in> || is_numzfc_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_numzf_boo{ is_numzf_cpt<t_in> };
    template<typename t_in>
    using is_numzf_v = v<is_numzf_boo<t_in>>;
    template<typename t_in>
    using is_numzf_v_t = t<is_numzf_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzf_stu {
        using type = is_numzf_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf_bind = bind<tem<is_numzf_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzf_t_cpt = is_t_cpt<t_in> && is_numzf_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzf_t_boo{ is_numzf_t_cpt<t_in> };
    template<typename t_in>
    using is_numzf_t_v = v<is_numzf_t_boo<t_in>>;
    template<typename t_in>
    using is_numzf_t_v_t = t<is_numzf_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzf_t_stu {
        using type = is_numzf_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf_t_bind = bind<tem<is_numzf_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numzf_v_cpt = is_v_cpt<t_in> && is_numzf_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numzf_v_boo{ is_numzf_v_cpt<t_in> };
    template<typename t_in>
    using is_numzf_v_v = v<is_numzf_v_boo<t_in>>;
    template<typename t_in>
    using is_numzf_v_v_t = t<is_numzf_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numzf_v_stu {
        using type = is_numzf_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numzf_v_bind = bind<tem<is_numzf_v_stu>>;
}

namespace lgo {
    template<typename>
    struct is_numzf_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<typename... t_in>
    struct is_numzf_cpt_stu<numzf<t_in...>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

namespace lgo {
    template<typename t_in>
    concept is_numzf_impl_cpt = is_numzf_cpt<t_in> || is_numzf_cpt_stu<t_in>::val;
}

#endif