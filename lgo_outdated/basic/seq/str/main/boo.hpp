#ifndef LGO_BASIC_SEQ_STR_MAIN_BOO
#define LGO_BASIC_SEQ_STR_MAIN_BOO

#include "cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_str_boo{ is_str_cpt<t_in> };
    template<typename t_in>
    using is_str_v = v<is_str_boo<t_in>>;
    template<typename t_in>
    using is_str_v_t = t<is_str_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_str_stu {
        using type = is_str_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_str_bind = bind<tem<is_str_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_str_t_boo{ is_str_t_cpt<t_in> };
    template<typename t_in>
    using is_str_t_v = v<is_str_t_boo<t_in>>;
    template<typename t_in>
    using is_str_t_v_t = t<is_str_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_str_t_stu {
        using type = is_str_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_str_t_bind = bind<tem<is_str_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_str_v_boo{ is_str_v_cpt<t_in> };
    template<typename t_in>
    using is_str_v_v = v<is_str_v_boo<t_in>>;
    template<typename t_in>
    using is_str_v_v_t = t<is_str_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_str_v_stu {
        using type = is_str_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_str_v_bind = bind<tem<is_str_v_stu>>;
}

#endif