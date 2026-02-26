#ifndef LGO_BASIC_SEQ_MEM_SUBD_BOO
#define LGO_BASIC_SEQ_MEM_SUBD_BOO

#include "cpt.hpp"

#include "../../../type/boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memsubd_boo{ is_memsubd_cpt<t_in> };
    template<typename t_in>
    using is_memsubd_v = v<is_memsubd_boo<t_in>>;
    template<typename t_in>
    using is_memsubd_v_t = t<is_memsubd_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memsubd_stu {
        using type = is_memsubd_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memsubd_bind = bind<tem<is_memsubd_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memsubd_t_boo{ is_memsubd_t_cpt<t_in> };
    template<typename t_in>
    using is_memsubd_t_v = v<is_memsubd_t_boo<t_in>>;
    template<typename t_in>
    using is_memsubd_t_v_t = t<is_memsubd_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memsubd_t_stu {
        using type = is_memsubd_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memsubd_t_bind = bind<tem<is_memsubd_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memsubd_v_boo{ is_memsubd_v_cpt<t_in> };
    template<typename t_in>
    using is_memsubd_v_v = v<is_memsubd_v_boo<t_in>>;
    template<typename t_in>
    using is_memsubd_v_v_t = t<is_memsubd_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memsubd_v_stu {
        using type = is_memsubd_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memsubd_v_bind = bind<tem<is_memsubd_v_stu>>;
}

#endif