#ifndef LGO_BASIC_TYPE_INT_P_BOO
#define LGO_BASIC_TYPE_INT_P_BOO

#include "cpt.hpp"

#include "../../boo/.hpp"
#include "../../tem/bind/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_intp_boo{ is_intp_cpt<t_in> };
    template<typename t_in>
    using is_intp_v = v<is_intp_boo<t_in>>;
    template<typename t_in>
    using is_intp_v_t = t<is_intp_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_intp_stu {
        using type = is_intp_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_intp_bind = bind<tem<is_intp_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_intp_t_boo{ is_intp_t_cpt<t_in> };
    template<typename t_in>
    using is_intp_t_v = v<is_intp_t_boo<t_in>>;
    template<typename t_in>
    using is_intp_t_v_t = t<is_intp_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_intp_t_stu {
        using type = is_intp_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_intp_t_bind = bind<tem<is_intp_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_intp_v_boo{ is_intp_v_cpt<t_in> };
    template<typename t_in>
    using is_intp_v_v = v<is_intp_v_boo<t_in>>;
    template<typename t_in>
    using is_intp_v_v_t = t<is_intp_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_intp_v_stu {
        using type = is_intp_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_intp_v_bind = bind<tem<is_intp_v_stu>>;
}

#endif