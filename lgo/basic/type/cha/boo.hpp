#ifndef LGO_BASIC_TYPE_CHA_BOO
#define LGO_BASIC_TYPE_CHA_BOO

#include "cpt.hpp"

#include "../boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_cha_boo{ is_cha_cpt<t_in> };
    template<typename t_in>
    using is_cha_v = v<is_cha_boo<t_in>>;
    template<typename t_in>
    using is_cha_v_t = t<is_cha_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_cha_stu {
        using type = is_cha_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_cha_bind = bind<tem<is_cha_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_cha_t_boo{ is_cha_t_cpt<t_in> };
    template<typename t_in>
    using is_cha_t_v = v<is_cha_t_boo<t_in>>;
    template<typename t_in>
    using is_cha_t_v_t = t<is_cha_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_cha_t_stu {
        using type = is_cha_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_cha_t_bind = bind<tem<is_cha_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_cha_v_boo{ is_cha_v_cpt<t_in> };
    template<typename t_in>
    using is_cha_v_v = v<is_cha_v_boo<t_in>>;
    template<typename t_in>
    using is_cha_v_v_t = t<is_cha_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_cha_v_stu {
        using type = is_cha_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_cha_v_bind = bind<tem<is_cha_v_stu>>;
}

#endif