#ifndef LGO_BASIC_SEQ_MEM_CPT_D_BOO
#define LGO_BASIC_SEQ_MEM_CPT_D_BOO

#include "d_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memd_boo{ is_memd_cpt<t_in> };
    template<typename t_in>
    using is_memd_v = v<is_memd_boo<t_in>>;
    template<typename t_in>
    using is_memd_v_t = t<is_memd_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memd_stu {
        using type = is_memd_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memd_bind = bind<tem<is_memd_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memd_t_boo{ is_memd_t_cpt<t_in> };
    template<typename t_in>
    using is_memd_t_v = v<is_memd_t_boo<t_in>>;
    template<typename t_in>
    using is_memd_t_v_t = t<is_memd_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memd_t_stu {
        using type = is_memd_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memd_t_bind = bind<tem<is_memd_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memd_v_boo{ is_memd_v_cpt<t_in> };
    template<typename t_in>
    using is_memd_v_v = v<is_memd_v_boo<t_in>>;
    template<typename t_in>
    using is_memd_v_v_t = t<is_memd_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memd_v_stu {
        using type = is_memd_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memd_v_bind = bind<tem<is_memd_v_stu>>;
}

#endif