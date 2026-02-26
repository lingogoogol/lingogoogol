#ifndef LGO_BASIC_SEQ_MAIN_C_CPT
#define LGO_BASIC_SEQ_MAIN_C_CPT

#include "../../arr/cpt/c.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqc_cpt = is_arrc_cpt<t_in> || is_strm_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_seqc_boo{ is_seqc_cpt<t_in> };
    template<typename t_in>
    using is_seqc_v = v<is_seqc_boo<t_in>>;
    template<typename t_in>
    using is_seqc_v_t = t<is_seqc_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqc_stu {
        using type = is_seqc_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqc_bind = bind<tem<is_seqc_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqc_t_cpt = is_t_cpt<t_in> && is_seqc_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqc_t_boo{ is_seqc_t_cpt<t_in> };
    template<typename t_in>
    using is_seqc_t_v = v<is_seqc_t_boo<t_in>>;
    template<typename t_in>
    using is_seqc_t_v_t = t<is_seqc_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqc_t_stu {
        using type = is_seqc_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqc_t_bind = bind<tem<is_seqc_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqc_v_cpt = is_v_cpt<t_in> && is_seqc_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqc_v_boo{ is_seqc_v_cpt<t_in> };
    template<typename t_in>
    using is_seqc_v_v = v<is_seqc_v_boo<t_in>>;
    template<typename t_in>
    using is_seqc_v_v_t = t<is_seqc_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqc_v_stu {
        using type = is_seqc_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqc_v_bind = bind<tem<is_seqc_v_stu>>;
}

namespace lgo {
    template<typename t_in>
    concept is_seqc_impl_cpt = is_arrc_cpt<t_in> || is_strm_impl_cpt<t_in>;
}

#endif