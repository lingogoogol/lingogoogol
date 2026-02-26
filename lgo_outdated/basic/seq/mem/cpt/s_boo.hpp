#ifndef LGO_BASIC_SEQ_MEM_CPT_S_BOO
#define LGO_BASIC_SEQ_MEM_CPT_S_BOO

#include "s_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_mems_boo{ is_mems_cpt<t_in> };
    template<typename t_in>
    using is_mems_v = v<is_mems_boo<t_in>>;
    template<typename t_in>
    using is_mems_v_t = t<is_mems_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_mems_stu {
        using type = is_mems_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_mems_bind = bind<tem<is_mems_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_mems_t_boo{ is_mems_t_cpt<t_in> };
    template<typename t_in>
    using is_mems_t_v = v<is_mems_t_boo<t_in>>;
    template<typename t_in>
    using is_mems_t_v_t = t<is_mems_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_mems_t_stu {
        using type = is_mems_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_mems_t_bind = bind<tem<is_mems_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_mems_v_boo{ is_mems_v_cpt<t_in> };
    template<typename t_in>
    using is_mems_v_v = v<is_mems_v_boo<t_in>>;
    template<typename t_in>
    using is_mems_v_v_t = t<is_mems_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_mems_v_stu {
        using type = is_mems_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_mems_v_bind = bind<tem<is_mems_v_stu>>;
}

#endif