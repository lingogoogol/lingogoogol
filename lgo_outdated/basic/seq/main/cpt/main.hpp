#ifndef LGO_BASIC_SEQ_MAIN_CPT_MAIN
#define LGO_BASIC_SEQ_MAIN_CPT_MAIN

#include "m.hpp"
#include "sub.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seq_cpt = is_seqm_cpt<t_in> || is_seqsub_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_seq_boo{ is_seq_cpt<t_in> };
    template<typename t_in>
    using is_seq_v = v<is_seq_boo<t_in>>;
    template<typename t_in>
    using is_seq_v_t = t<is_seq_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seq_stu {
        using type = is_seq_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seq_bind = bind<tem<is_seq_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seq_t_cpt = is_t_cpt<t_in> && is_seq_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seq_t_boo{ is_seq_t_cpt<t_in> };
    template<typename t_in>
    using is_seq_t_v = v<is_seq_t_boo<t_in>>;
    template<typename t_in>
    using is_seq_t_v_t = t<is_seq_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seq_t_stu {
        using type = is_seq_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seq_t_bind = bind<tem<is_seq_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seq_v_cpt = is_v_cpt<t_in> && is_seq_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seq_v_boo{ is_seq_v_cpt<t_in> };
    template<typename t_in>
    using is_seq_v_v = v<is_seq_v_boo<t_in>>;
    template<typename t_in>
    using is_seq_v_v_t = t<is_seq_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seq_v_stu {
        using type = is_seq_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seq_v_bind = bind<tem<is_seq_v_stu>>;
}

namespace lgo {
    template<typename t_in>
    concept is_seq_impl_cpt = is_seqm_impl_cpt<t_in> || is_seqsub_cpt<t_in>;
}

#endif