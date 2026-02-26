#ifndef LGO_BASIC_SEQ_MEM_CPT_M_BOO
#define LGO_BASIC_SEQ_MEM_CPT_M_BOO

#include "m_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memm_boo{ is_memm_cpt<t_in> };
    template<typename t_in>
    using is_memm_v = v<is_memm_boo<t_in>>;
    template<typename t_in>
    using is_memm_v_t = t<is_memm_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memm_stu {
        using type = is_memm_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memm_bind = bind<tem<is_memm_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memm_t_boo{ is_memm_t_cpt<t_in> };
    template<typename t_in>
    using is_memm_t_v = v<is_memm_t_boo<t_in>>;
    template<typename t_in>
    using is_memm_t_v_t = t<is_memm_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memm_t_stu {
        using type = is_memm_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memm_t_bind = bind<tem<is_memm_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memm_v_boo{ is_memm_v_cpt<t_in> };
    template<typename t_in>
    using is_memm_v_v = v<is_memm_v_boo<t_in>>;
    template<typename t_in>
    using is_memm_v_v_t = t<is_memm_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memm_v_stu {
        using type = is_memm_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memm_v_bind = bind<tem<is_memm_v_stu>>;
}

#endif