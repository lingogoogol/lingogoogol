#ifndef LGO_BASIC_SEQ_MAIN_CPT_M
#define LGO_BASIC_SEQ_MAIN_CPT_M

#include "../../arr/cpt/m.hpp"
#include "../../str/cpt/m_cpt.hpp"
#include "../../mem/cpt/m_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqm_cpt = is_arrm_cpt<t_in> || is_strm_cpt<t_in> || is_memm_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_seqm_boo{ is_seqm_cpt<t_in> };
    template<typename t_in>
    using is_seqm_v = v<is_seqm_boo<t_in>>;
    template<typename t_in>
    using is_seqm_v_t = t<is_seqm_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqm_stu {
        using type = is_seqm_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqm_bind = bind<tem<is_seqm_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqm_t_cpt = is_t_cpt<t_in> && is_seqm_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqm_t_boo{ is_seqm_t_cpt<t_in> };
    template<typename t_in>
    using is_seqm_t_v = v<is_seqm_t_boo<t_in>>;
    template<typename t_in>
    using is_seqm_t_v_t = t<is_seqm_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqm_t_stu {
        using type = is_seqm_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqm_t_bind = bind<tem<is_seqm_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqm_v_cpt = is_v_cpt<t_in> && is_seqm_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqm_v_boo{ is_seqm_v_cpt<t_in> };
    template<typename t_in>
    using is_seqm_v_v = v<is_seqm_v_boo<t_in>>;
    template<typename t_in>
    using is_seqm_v_v_t = t<is_seqm_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqm_v_stu {
        using type = is_seqm_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqm_v_bind = bind<tem<is_seqm_v_stu>>;
}

namespace lgo {
    template<typename t_in>
    concept is_seqm_impl_cpt = is_arrm_cpt<t_in>
    || is_strm_impl_cpt<t_in> || is_memm_cpt<t_in>;
}

#endif