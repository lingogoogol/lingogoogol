#ifndef LGO_BASIC_SEQ_IO_CPT_M
#define LGO_BASIC_SEQ_IO_CPT_M

#include "../../str/cpt/m_cpt.hpp"
#include "../../mem/cpt/m_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqiom_cpt = is_strm_cpt<t_in> || is_memm_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_seqiom_boo{ is_seqiom_cpt<t_in> };
    template<typename t_in>
    using is_seqiom_v = v<is_seqiom_boo<t_in>>;
    template<typename t_in>
    using is_seqiom_v_t = t<is_seqiom_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqiom_stu {
        using type = is_seqiom_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqiom_bind = bind<tem<is_seqiom_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqiom_t_cpt = is_t_cpt<t_in> && is_seqiom_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqiom_t_boo{ is_seqiom_t_cpt<t_in> };
    template<typename t_in>
    using is_seqiom_t_v = v<is_seqiom_t_boo<t_in>>;
    template<typename t_in>
    using is_seqiom_t_v_t = t<is_seqiom_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqiom_t_stu {
        using type = is_seqiom_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqiom_t_bind = bind<tem<is_seqiom_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqiom_v_cpt = is_v_cpt<t_in> && is_seqiom_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqiom_v_boo{ is_seqiom_v_cpt<t_in> };
    template<typename t_in>
    using is_seqiom_v_v = v<is_seqiom_v_boo<t_in>>;
    template<typename t_in>
    using is_seqiom_v_v_t = t<is_seqiom_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqiom_v_stu {
        using type = is_seqiom_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqiom_v_bind = bind<tem<is_seqiom_v_stu>>;
}

#endif