#ifndef LGO_BASIC_SEQ_STR_CPT_S_BOO
#define LGO_BASIC_SEQ_STR_CPT_S_BOO

#include "s_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strs_boo{ is_strs_cpt<t_in> };
    template<typename t_in>
    using is_strs_v = v<is_strs_boo<t_in>>;
    template<typename t_in>
    using is_strs_v_t = t<is_strs_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strs_stu {
        using type = is_strs_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strs_bind = bind<tem<is_strs_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strs_t_boo{ is_strs_t_cpt<t_in> };
    template<typename t_in>
    using is_strs_t_v = v<is_strs_t_boo<t_in>>;
    template<typename t_in>
    using is_strs_t_v_t = t<is_strs_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strs_t_stu {
        using type = is_strs_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strs_t_bind = bind<tem<is_strs_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strs_v_boo{ is_strs_v_cpt<t_in> };
    template<typename t_in>
    using is_strs_v_v = v<is_strs_v_boo<t_in>>;
    template<typename t_in>
    using is_strs_v_v_t = t<is_strs_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strs_v_stu {
        using type = is_strs_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strs_v_bind = bind<tem<is_strs_v_stu>>;
}

#endif