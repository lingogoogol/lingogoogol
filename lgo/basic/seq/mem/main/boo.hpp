#ifndef LGO_BASIC_SEQ_MEM_MAIN_BOO
#define LGO_BASIC_SEQ_MEM_MAIN_BOO

#include "cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_mem_boo{ is_mem_cpt<t_in> };
    template<typename t_in>
    using is_mem_v = v<is_mem_boo<t_in>>;
    template<typename t_in>
    using is_mem_v_t = t<is_mem_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_mem_stu {
        using type = is_mem_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_mem_bind = bind<tem<is_mem_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_mem_t_boo{ is_mem_t_cpt<t_in> };
    template<typename t_in>
    using is_mem_t_v = v<is_mem_t_boo<t_in>>;
    template<typename t_in>
    using is_mem_t_v_t = t<is_mem_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_mem_t_stu {
        using type = is_mem_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_mem_t_bind = bind<tem<is_mem_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_mem_v_boo{ is_mem_v_cpt<t_in> };
    template<typename t_in>
    using is_mem_v_v = v<is_mem_v_boo<t_in>>;
    template<typename t_in>
    using is_mem_v_v_t = t<is_mem_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_mem_v_stu {
        using type = is_mem_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_mem_v_bind = bind<tem<is_mem_v_stu>>;
}

#endif