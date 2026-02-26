#ifndef LGO_BASIC_SEQ_STR_MD_BOO
#define LGO_BASIC_SEQ_STR_MD_BOO

#include "cpt.hpp"

#include "../../../type/boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strmd_boo{ is_strmd_cpt<t_in> };
    template<typename t_in>
    using is_strmd_v = v<is_strmd_boo<t_in>>;
    template<typename t_in>
    using is_strmd_v_t = t<is_strmd_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strmd_stu {
        using type = is_strmd_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strmd_bind = bind<tem<is_strmd_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strmd_t_boo{ is_strmd_t_cpt<t_in> };
    template<typename t_in>
    using is_strmd_t_v = v<is_strmd_t_boo<t_in>>;
    template<typename t_in>
    using is_strmd_t_v_t = t<is_strmd_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strmd_t_stu {
        using type = is_strmd_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strmd_t_bind = bind<tem<is_strmd_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strmd_v_boo{ is_strmd_v_cpt<t_in> };
    template<typename t_in>
    using is_strmd_v_v = v<is_strmd_v_boo<t_in>>;
    template<typename t_in>
    using is_strmd_v_v_t = t<is_strmd_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strmd_v_stu {
        using type = is_strmd_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strmd_v_bind = bind<tem<is_strmd_v_stu>>;
}

#endif