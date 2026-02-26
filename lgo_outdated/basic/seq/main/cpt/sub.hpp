#ifndef LGO_BASIC_SEQ_MAIN_CPT_SUB
#define LGO_BASIC_SEQ_MAIN_CPT_SUB

#include "../../arr/cpt/sub.hpp"
#include "../../str/cpt/sub_cpt.hpp"
#include "../../mem/cpt/sub_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqsub_cpt = is_arrsub_cpt<t_in> || is_strsub_cpt<t_in> || is_memsub_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_seqsub_boo{ is_seqsub_cpt<t_in> };
    template<typename t_in>
    using is_seqsub_v = v<is_seqsub_boo<t_in>>;
    template<typename t_in>
    using is_seqsub_v_t = t<is_seqsub_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqsub_stu {
        using type = is_seqsub_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqsub_bind = bind<tem<is_seqsub_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqsub_t_cpt = is_t_cpt<t_in> && is_seqsub_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqsub_t_boo{ is_seqsub_t_cpt<t_in> };
    template<typename t_in>
    using is_seqsub_t_v = v<is_seqsub_t_boo<t_in>>;
    template<typename t_in>
    using is_seqsub_t_v_t = t<is_seqsub_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqsub_t_stu {
        using type = is_seqsub_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqsub_t_bind = bind<tem<is_seqsub_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqsub_v_cpt = is_v_cpt<t_in> && is_seqsub_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqsub_v_boo{ is_seqsub_v_cpt<t_in> };
    template<typename t_in>
    using is_seqsub_v_v = v<is_seqsub_v_boo<t_in>>;
    template<typename t_in>
    using is_seqsub_v_v_t = t<is_seqsub_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqsub_v_stu {
        using type = is_seqsub_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqsub_v_bind = bind<tem<is_seqsub_v_stu>>;
}

#endif