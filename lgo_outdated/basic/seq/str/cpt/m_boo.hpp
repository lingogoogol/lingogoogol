#ifndef LGO_BASIC_SEQ_STR_CPT_M_BOO
#define LGO_BASIC_SEQ_STR_CPT_M_BOO

#include "m_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strm_boo{ is_strm_cpt<t_in> };
    template<typename t_in>
    using is_strm_v = v<is_strm_boo<t_in>>;
    template<typename t_in>
    using is_strm_v_t = t<is_strm_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strm_stu {
        using type = is_strm_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strm_bind = bind<tem<is_strm_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strm_t_boo{ is_strm_t_cpt<t_in> };
    template<typename t_in>
    using is_strm_t_v = v<is_strm_t_boo<t_in>>;
    template<typename t_in>
    using is_strm_t_v_t = t<is_strm_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strm_t_stu {
        using type = is_strm_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strm_t_bind = bind<tem<is_strm_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strm_v_boo{ is_strm_v_cpt<t_in> };
    template<typename t_in>
    using is_strm_v_v = v<is_strm_v_boo<t_in>>;
    template<typename t_in>
    using is_strm_v_v_t = t<is_strm_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strm_v_stu {
        using type = is_strm_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strm_v_bind = bind<tem<is_strm_v_stu>>;
}

#endif