#ifndef LGO_BASIC_SEQ_MEM_CPT_SUB_BOO
#define LGO_BASIC_SEQ_MEM_CPT_SUB_BOO

#include "sub_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memsub_boo{ is_memsub_cpt<t_in> };
    template<typename t_in>
    using is_memsub_v = v<is_memsub_boo<t_in>>;
    template<typename t_in>
    using is_memsub_v_t = t<is_memsub_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memsub_stu {
        using type = is_memsub_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memsub_bind = bind<tem<is_memsub_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memsub_t_boo{ is_memsub_t_cpt<t_in> };
    template<typename t_in>
    using is_memsub_t_v = v<is_memsub_t_boo<t_in>>;
    template<typename t_in>
    using is_memsub_t_v_t = t<is_memsub_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memsub_t_stu {
        using type = is_memsub_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memsub_t_bind = bind<tem<is_memsub_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memsub_v_boo{ is_memsub_v_cpt<t_in> };
    template<typename t_in>
    using is_memsub_v_v = v<is_memsub_v_boo<t_in>>;
    template<typename t_in>
    using is_memsub_v_v_t = t<is_memsub_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memsub_v_stu {
        using type = is_memsub_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memsub_v_bind = bind<tem<is_memsub_v_stu>>;
}

#endif