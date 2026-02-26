#ifndef LGO_BASIC_SEQ_STR_MS_BOO
#define LGO_BASIC_SEQ_STR_MS_BOO

#include "cpt.hpp"

#include "../../../type/boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strms8_boo{ is_strms8_cpt<t_in> };
    template<typename t_in>
    using is_strms8_v = v<is_strms8_boo<t_in>>;
    template<typename t_in>
    using is_strms8_v_t = t<is_strms8_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strms8_stu {
        using type = is_strms8_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strms8_bind = bind<tem<is_strms8_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strms8_t_boo{ is_strms8_t_cpt<t_in> };
    template<typename t_in>
    using is_strms8_t_v = v<is_strms8_t_boo<t_in>>;
    template<typename t_in>
    using is_strms8_t_v_t = t<is_strms8_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strms8_t_stu {
        using type = is_strms8_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strms8_t_bind = bind<tem<is_strms8_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strms8_v_boo{ is_strms8_v_cpt<t_in> };
    template<typename t_in>
    using is_strms8_v_v = v<is_strms8_v_boo<t_in>>;
    template<typename t_in>
    using is_strms8_v_v_t = t<is_strms8_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strms8_v_stu {
        using type = is_strms8_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strms8_v_bind = bind<tem<is_strms8_v_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strms16_boo{ is_strms16_cpt<t_in> };
    template<typename t_in>
    using is_strms16_v = v<is_strms16_boo<t_in>>;
    template<typename t_in>
    using is_strms16_v_t = t<is_strms16_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strms16_stu {
        using type = is_strms16_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strms16_bind = bind<tem<is_strms16_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strms16_t_boo{ is_strms16_t_cpt<t_in> };
    template<typename t_in>
    using is_strms16_t_v = v<is_strms16_t_boo<t_in>>;
    template<typename t_in>
    using is_strms16_t_v_t = t<is_strms16_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strms16_t_stu {
        using type = is_strms16_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strms16_t_bind = bind<tem<is_strms16_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strms16_v_boo{ is_strms16_v_cpt<t_in> };
    template<typename t_in>
    using is_strms16_v_v = v<is_strms16_v_boo<t_in>>;
    template<typename t_in>
    using is_strms16_v_v_t = t<is_strms16_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strms16_v_stu {
        using type = is_strms16_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strms16_v_bind = bind<tem<is_strms16_v_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strms32_boo{ is_strms32_cpt<t_in> };
    template<typename t_in>
    using is_strms32_v = v<is_strms32_boo<t_in>>;
    template<typename t_in>
    using is_strms32_v_t = t<is_strms32_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strms32_stu {
        using type = is_strms32_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strms32_bind = bind<tem<is_strms32_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strms32_t_boo{ is_strms32_t_cpt<t_in> };
    template<typename t_in>
    using is_strms32_t_v = v<is_strms32_t_boo<t_in>>;
    template<typename t_in>
    using is_strms32_t_v_t = t<is_strms32_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strms32_t_stu {
        using type = is_strms32_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strms32_t_bind = bind<tem<is_strms32_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strms32_v_boo{ is_strms32_v_cpt<t_in> };
    template<typename t_in>
    using is_strms32_v_v = v<is_strms32_v_boo<t_in>>;
    template<typename t_in>
    using is_strms32_v_v_t = t<is_strms32_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strms32_v_stu {
        using type = is_strms32_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strms32_v_bind = bind<tem<is_strms32_v_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strmsc_boo{ is_strmsc_cpt<t_in> };
    template<typename t_in>
    using is_strmsc_v = v<is_strmsc_boo<t_in>>;
    template<typename t_in>
    using is_strmsc_v_t = t<is_strmsc_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strmsc_stu {
        using type = is_strmsc_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strmsc_bind = bind<tem<is_strmsc_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strmsc_t_boo{ is_strmsc_t_cpt<t_in> };
    template<typename t_in>
    using is_strmsc_t_v = v<is_strmsc_t_boo<t_in>>;
    template<typename t_in>
    using is_strmsc_t_v_t = t<is_strmsc_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strmsc_t_stu {
        using type = is_strmsc_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strmsc_t_bind = bind<tem<is_strmsc_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strmsc_v_boo{ is_strmsc_v_cpt<t_in> };
    template<typename t_in>
    using is_strmsc_v_v = v<is_strmsc_v_boo<t_in>>;
    template<typename t_in>
    using is_strmsc_v_v_t = t<is_strmsc_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strmsc_v_stu {
        using type = is_strmsc_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strmsc_v_bind = bind<tem<is_strmsc_v_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strms_boo{ is_strms_cpt<t_in> };
    template<typename t_in>
    using is_strms_v = v<is_strms_boo<t_in>>;
    template<typename t_in>
    using is_strms_v_t = t<is_strms_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strms_stu {
        using type = is_strms_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strms_bind = bind<tem<is_strms_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strms_t_boo{ is_strms_t_cpt<t_in> };
    template<typename t_in>
    using is_strms_t_v = v<is_strms_t_boo<t_in>>;
    template<typename t_in>
    using is_strms_t_v_t = t<is_strms_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strms_t_stu {
        using type = is_strms_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strms_t_bind = bind<tem<is_strms_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_strms_v_boo{ is_strms_v_cpt<t_in> };
    template<typename t_in>
    using is_strms_v_v = v<is_strms_v_boo<t_in>>;
    template<typename t_in>
    using is_strms_v_v_t = t<is_strms_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_strms_v_stu {
        using type = is_strms_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_strms_v_bind = bind<tem<is_strms_v_stu>>;
}

#endif