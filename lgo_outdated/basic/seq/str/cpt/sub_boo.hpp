#ifndef LGO_BASIC_SEQ_STR_CPT_SUB_BOO
#define LGO_BASIC_SEQ_STR_CPT_SUB_BOO

#include "sub_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strsub_boo{ is_strsub_cpt<t_in> };
    template<typename t_in>
    using is_strsub_v = v<is_strsub_boo<t_in>>;
    template<typename t_in>
    using is_strsub_v_t = t<is_strsub_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strsub_stu {
        using type = is_strsub_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strsub_bind = bind<tem<is_strsub_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strsub_t_boo{ is_strsub_t_cpt<t_in> };
    template<typename t_in>
    using is_strsub_t_v = v<is_strsub_t_boo<t_in>>;
    template<typename t_in>
    using is_strsub_t_v_t = t<is_strsub_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strsub_t_stu {
        using type = is_strsub_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strsub_t_bind = bind<tem<is_strsub_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strsub_v_boo{ is_strsub_v_cpt<t_in> };
    template<typename t_in>
    using is_strsub_v_v = v<is_strsub_v_boo<t_in>>;
    template<typename t_in>
    using is_strsub_v_v_t = t<is_strsub_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strsub_v_stu {
        using type = is_strsub_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strsub_v_bind = bind<tem<is_strsub_v_stu>>;
}

#endif