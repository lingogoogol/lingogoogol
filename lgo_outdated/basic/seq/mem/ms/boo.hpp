#ifndef LGO_BASIC_SEQ_MEM_MS_BOO
#define LGO_BASIC_SEQ_MEM_MS_BOO

#include "cpt.hpp"

#include "../../../type/boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memms_boo{ is_memms_cpt<t_in> };
    template<typename t_in>
    using is_memms_v = v<is_memms_boo<t_in>>;
    template<typename t_in>
    using is_memms_v_t = t<is_memms_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memms_stu {
        using type = is_memms_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memms_bind = bind<tem<is_memms_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memms_t_boo{ is_memms_t_cpt<t_in> };
    template<typename t_in>
    using is_memms_t_v = v<is_memms_t_boo<t_in>>;
    template<typename t_in>
    using is_memms_t_v_t = t<is_memms_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memms_t_stu {
        using type = is_memms_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memms_t_bind = bind<tem<is_memms_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memms_v_boo{ is_memms_v_cpt<t_in> };
    template<typename t_in>
    using is_memms_v_v = v<is_memms_v_boo<t_in>>;
    template<typename t_in>
    using is_memms_v_v_t = t<is_memms_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memms_v_stu {
        using type = is_memms_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memms_v_bind = bind<tem<is_memms_v_stu>>;
}

#endif