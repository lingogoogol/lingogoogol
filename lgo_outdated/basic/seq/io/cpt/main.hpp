#ifndef LGO_BASIC_SEQ_IO_CPT_MAIN
#define LGO_BASIC_SEQ_IO_CPT_MAIN

#include "m.hpp"
#include "sub.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqio_cpt = is_seqiom_cpt<t_in> || is_seqiosub_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_seqio_boo{ is_seqio_cpt<t_in> };
    template<typename t_in>
    using is_seqio_v = v<is_seqio_boo<t_in>>;
    template<typename t_in>
    using is_seqio_v_t = t<is_seqio_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqio_stu {
        using type = is_seqio_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqio_bind = bind<tem<is_seqio_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqio_t_cpt = is_t_cpt<t_in> && is_seqio_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqio_t_boo{ is_seqio_t_cpt<t_in> };
    template<typename t_in>
    using is_seqio_t_v = v<is_seqio_t_boo<t_in>>;
    template<typename t_in>
    using is_seqio_t_v_t = t<is_seqio_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqio_t_stu {
        using type = is_seqio_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqio_t_bind = bind<tem<is_seqio_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqio_v_cpt = is_v_cpt<t_in> && is_seqio_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqio_v_boo{ is_seqio_v_cpt<t_in> };
    template<typename t_in>
    using is_seqio_v_v = v<is_seqio_v_boo<t_in>>;
    template<typename t_in>
    using is_seqio_v_v_t = t<is_seqio_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqio_v_stu {
        using type = is_seqio_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqio_v_bind = bind<tem<is_seqio_v_stu>>;
}

#endif