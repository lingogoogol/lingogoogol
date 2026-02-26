#ifndef LGO_BASIC_SEQ_STR_SUBS_BOO
#define LGO_BASIC_SEQ_STR_SUBS_BOO

#include "cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strsubs_boo{ is_strsubs_cpt<t_in> };
    template<typename t_in>
    using is_strsubs_v = v<is_strsubs_boo<t_in>>;
    template<typename t_in>
    using is_strsubs_v_t = t<is_strsubs_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strsubs_stu {
        using type = is_strsubs_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strsubs_bind = bind<tem<is_strsubs_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strsubs_t_boo{ is_strsubs_t_cpt<t_in> };
    template<typename t_in>
    using is_strsubs_t_v = v<is_strsubs_t_boo<t_in>>;
    template<typename t_in>
    using is_strsubs_t_v_t = t<is_strsubs_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strsubs_t_stu {
        using type = is_strsubs_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strsubs_t_bind = bind<tem<is_strsubs_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strsubs_v_boo{ is_strsubs_v_cpt<t_in> };
    template<typename t_in>
    using is_strsubs_v_v = v<is_strsubs_v_boo<t_in>>;
    template<typename t_in>
    using is_strsubs_v_v_t = t<is_strsubs_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strsubs_v_stu {
        using type = is_strsubs_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strsubs_v_bind = bind<tem<is_strsubs_v_stu>>;
}

#endif