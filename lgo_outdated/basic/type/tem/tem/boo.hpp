#ifndef LGO_BASIC_TYPE_TEM_TEM_BOO
#define LGO_BASIC_TYPE_TEM_TEM_BOO

#include "cpt.hpp"

#include "../../boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_tem_boo{ is_tem_cpt<t_in> };
    template<typename t_in>
    using is_tem_v = v<is_tem_boo<t_in>>;
    template<typename t_in>
    using is_tem_v_t = t<is_tem_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_tem_stu {
        using type = v<is_tem_boo<t_in>>;
    };
}

LGO_EXPORT namespace lgo {
    using is_tem_bind = bind<tem<is_tem_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_tem_t_boo{ is_tem_t_cpt<t_in> };
    template<typename t_in>
    using is_tem_t_v = v<is_tem_t_boo<t_in>>;
    template<typename t_in>
    using is_tem_t_v_t = t<is_tem_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_tem_t_stu {
        using type = is_tem_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_tem_t_bind = bind<tem<is_tem_t_stu>>;
}

#endif