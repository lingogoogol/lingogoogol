#ifndef LGO_BASIC_TYPE_INT_MAIN_OPT_CPT_ENDIAN
#define LGO_BASIC_TYPE_INT_MAIN_OPT_CPT_ENDIAN

#include "../little_endian/.hpp"
#include "../big_endian/.hpp"
#include "../../../../tem/tem/def.hpp"
#include "../../../../tem/bind/decl.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_int_endian_cpt = is_same_cpt<t_in, int_little_endian> || is_same_cpt<t_in, int_big_endian>;
    template<typename t_in>
    constexpr inline boo is_int_endian_boo{ is_int_endian_cpt<t_in> };
    template<typename t_in>
    using is_int_endian_v = v<is_int_endian_boo<t_in>>;
    template<typename t_in>
    using is_int_endian_v_t = t<is_int_endian_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_int_endian_stu {
        using type = is_int_endian_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_int_endian_bind = bind<tem<is_int_endian_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_int_endian_t_cpt = is_t_cpt<t_in> && is_int_endian_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_int_endian_t_boo{ is_int_endian_t_cpt<t_in> };
    template<typename t_in>
    using is_int_endian_t_v = v<is_int_endian_t_boo<t_in>>;
    template<typename t_in>
    using is_int_endian_t_v_t = t<is_int_endian_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_int_endian_t_stu {
        using type = is_int_endian_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_int_endian_t_bind = bind<tem<is_int_endian_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_int_endian_v_cpt = is_v_cpt<t_in> && is_int_endian_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_int_endian_v_boo{ is_int_endian_v_cpt<t_in> };
    template<typename t_in>
    using is_int_endian_v_v = v<is_int_endian_v_boo<t_in>>;
    template<typename t_in>
    using is_int_endian_v_v_t = t<is_int_endian_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_int_endian_v_stu {
        using type = is_int_endian_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_int_endian_v_bind = bind<tem<is_int_endian_v_stu>>;
}

#endif