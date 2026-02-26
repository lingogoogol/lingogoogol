#ifndef LGO_BASIC_SEQ_MAIN_D_CPT
#define LGO_BASIC_SEQ_MAIN_D_CPT

#include "../../arr/cpt/d.hpp"
#include "../../str/cpt/d_cpt.hpp"
#include "../../mem/cpt/d_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqd_cpt = is_arrd_cpt<t_in> || is_strd_cpt<t_in> || is_memd_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_seqd_boo{ is_seqd_cpt<t_in> };
    template<typename t_in>
    using is_seqd_v = v<is_seqd_boo<t_in>>;
    template<typename t_in>
    using is_seqd_v_t = t<is_seqd_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqd_stu {
        using type = is_seqd_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqd_bind = bind<tem<is_seqd_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqd_t_cpt = is_t_cpt<t_in> && is_seqd_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqd_t_boo{ is_seqd_t_cpt<t_in> };
    template<typename t_in>
    using is_seqd_t_v = v<is_seqd_t_boo<t_in>>;
    template<typename t_in>
    using is_seqd_t_v_t = t<is_seqd_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqd_t_stu {
        using type = is_seqd_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqd_t_bind = bind<tem<is_seqd_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqd_v_cpt = is_v_cpt<t_in> && is_seqd_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqd_v_boo{ is_seqd_v_cpt<t_in> };
    template<typename t_in>
    using is_seqd_v_v = v<is_seqd_v_boo<t_in>>;
    template<typename t_in>
    using is_seqd_v_v_t = t<is_seqd_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqd_v_stu {
        using type = is_seqd_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqd_v_bind = bind<tem<is_seqd_v_stu>>;
}

#endif