#ifndef LGO_BASIC_IO_OPT_UNTIL_IGNORE_CPT
#define LGO_BASIC_IO_OPT_UNTIL_IGNORE_CPT

#include "def.hpp"

namespace lgo {
    template<typename>
    struct is_io_until_ignore_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_io_until_ignore_stu<io_until_ignore<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_io_until_ignore_bind = bind<tem<is_io_until_ignore_stu>>;
    template<typename t_in>
    using is_io_until_ignore_v = is_io_until_ignore_bind::type<t_in>;
    template<typename t_in>
    using is_io_until_ignore_v_t = t<is_io_until_ignore_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_io_until_ignore_boo{ is_io_until_ignore_v<t_in>::val };
    template<typename t_in>
    concept is_io_until_ignore_cpt = is_io_until_ignore_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_io_until_ignore_t_cpt = is_t_cpt<t_in> && is_io_until_ignore_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_io_until_ignore_t_boo{ is_io_until_ignore_t_cpt<t_in> };
    template<typename t_in>
    using is_io_until_ignore_t_v = v<is_io_until_ignore_t_boo<t_in>>;
    template<typename t_in>
    using is_io_until_ignore_t_v_t = t<is_io_until_ignore_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_io_until_ignore_t_stu {
        using type = is_io_until_ignore_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_io_until_ignore_t_bind = bind<tem<is_io_until_ignore_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_io_until_ignore_v_cpt = is_v_cpt<t_in> && is_io_until_ignore_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_io_until_ignore_v_boo{ is_io_until_ignore_v_cpt<t_in>::val };
    template<typename t_in>
    using is_io_until_ignore_v_v = v<is_io_until_ignore_v_boo<t_in>>;
    template<typename t_in>
    using is_io_until_ignore_v_v_t = t<is_io_until_ignore_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_io_until_ignore_v_stu {
        using type = is_io_until_ignore_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_io_until_ignore_v_bind = bind<tem<is_io_until_ignore_v_stu>>;
}

#endif