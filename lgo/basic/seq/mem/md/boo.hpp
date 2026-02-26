#ifndef LGO_BASIC_SEQ_MEM_MD_BOO
#define LGO_BASIC_SEQ_MEM_MD_BOO

#include "cpt.hpp"

#include "../../../type/boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memmd_boo{ is_memmd_cpt<t_in> };
    template<typename t_in>
    using is_memmd_v = v<is_memmd_boo<t_in>>;
    template<typename t_in>
    using is_memmd_v_t = t<is_memmd_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memmd_stu {
        using type = is_memmd_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memmd_bind = bind<tem<is_memmd_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memmd_t_boo{ is_memmd_t_cpt<t_in> };
    template<typename t_in>
    using is_memmd_t_v = v<is_memmd_t_boo<t_in>>;
    template<typename t_in>
    using is_memmd_t_v_t = t<is_memmd_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memmd_t_stu {
        using type = is_memmd_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memmd_t_bind = bind<tem<is_memmd_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memmd_v_boo{ is_memmd_v_cpt<t_in> };
    template<typename t_in>
    using is_memmd_v_v = v<is_memmd_v_boo<t_in>>;
    template<typename t_in>
    using is_memmd_v_v_t = t<is_memmd_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memmd_v_stu {
        using type = is_memmd_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memmd_v_bind = bind<tem<is_memmd_v_stu>>;
}

#endif