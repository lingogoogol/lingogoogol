#ifndef LGO_IO_CPT
#define LGO_IO_CPT

#include "cmd/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_io_cpt = is_same_cpt<t_in, cmd>;
    template<typename t_in>
    constexpr inline boo is_io_boo{ is_io_cpt<t_in> };
    template<typename t_in>
    using is_io_v = v<is_io_boo<t_in>>;
    template<typename t_in>
    using is_io_v_t = t<is_io_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_io_stu {
        using type = is_io_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_io_bind = bind<tem<is_io_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_io_t_cpt = is_t_cpt<t_in> && is_io_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_io_t_boo{ is_io_t_cpt<t_in> };
    template<typename t_in>
    using is_io_t_v = v<is_io_t_boo<t_in>>;
    template<typename t_in>
    using is_io_t_v_t = t<is_io_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_io_t_stu {
        using type = is_io_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_io_t_bind = bind<tem<is_io_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_io_v_cpt = is_v_cpt<t_in> && is_io_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_io_v_boo{ is_io_v_cpt<t_in> };
    template<typename t_in>
    using is_io_v_v = v<is_io_v_boo<t_in>>;
    template<typename t_in>
    using is_io_v_v_t = t<is_io_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_io_v_stu {
        using type = is_io_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_io_v_bind = bind<tem<is_io_v_stu>>;
}

#endif