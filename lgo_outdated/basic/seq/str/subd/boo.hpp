#ifndef LGO_BASIC_SEQ_STR_SUBD_BOO
#define LGO_BASIC_SEQ_STR_SUBD_BOO

#include "cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strsubd_boo{ is_strsubd_cpt<t_in> };
    template<typename t_in>
    using is_strsubd_v = v<is_strsubd_boo<t_in>>;
    template<typename t_in>
    using is_strsubd_v_t = t<is_strsubd_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strsubd_stu {
        using type = is_strsubd_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strsubd_bind = bind<tem<is_strsubd_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strsubd_t_boo{ is_strsubd_t_cpt<t_in> };
    template<typename t_in>
    using is_strsubd_t_v = v<is_strsubd_t_boo<t_in>>;
    template<typename t_in>
    using is_strsubd_t_v_t = t<is_strsubd_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strsubd_t_stu {
        using type = is_strsubd_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strsubd_t_bind = bind<tem<is_strsubd_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strsubd_v_boo{ is_strsubd_v_cpt<t_in> };
    template<typename t_in>
    using is_strsubd_v_v = v<is_strsubd_v_boo<t_in>>;
    template<typename t_in>
    using is_strsubd_v_v_t = t<is_strsubd_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strsubd_v_stu {
        using type = is_strsubd_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strsubd_v_bind = bind<tem<is_strsubd_v_stu>>;
}

#endif