#ifndef LGO_BASIC_SEQ_MEM_SUBS_BOO
#define LGO_BASIC_SEQ_MEM_SUBS_BOO

#include "cpt.hpp"

#include "../../../type/boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memsubs_boo{ is_memsubs_cpt<t_in> };
    template<typename t_in>
    using is_memsubs_v = v<is_memsubs_boo<t_in>>;
    template<typename t_in>
    using is_memsubs_v_t = t<is_memsubs_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memsubs_stu {
        using type = is_memsubs_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memsubs_bind = bind<tem<is_memsubs_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memsubs_t_boo{ is_memsubs_t_cpt<t_in> };
    template<typename t_in>
    using is_memsubs_t_v = v<is_memsubs_t_boo<t_in>>;
    template<typename t_in>
    using is_memsubs_t_v_t = t<is_memsubs_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memsubs_t_stu {
        using type = is_memsubs_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memsubs_t_bind = bind<tem<is_memsubs_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_memsubs_v_boo{ is_memsubs_v_cpt<t_in> };
    template<typename t_in>
    using is_memsubs_v_v = v<is_memsubs_v_boo<t_in>>;
    template<typename t_in>
    using is_memsubs_v_v_t = t<is_memsubs_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_memsubs_v_stu {
        using type = is_memsubs_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_memsubs_v_bind = bind<tem<is_memsubs_v_stu>>;
}

#endif