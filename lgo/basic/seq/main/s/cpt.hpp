#ifndef LGO_BASIC_SEQ_MAIN_S_CPT
#define LGO_BASIC_SEQ_MAIN_S_CPT

#include "../../arr/cpt/s.hpp"
#include "../../str/cpt/s_cpt.hpp"
#include "../../mem/cpt/s_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqs_cpt = is_arrs_cpt<t_in> || is_strs_cpt<t_in> || is_mems_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_seqs_boo{ is_seqs_cpt<t_in> };
    template<typename t_in>
    using is_seqs_v = v<is_seqs_boo<t_in>>;
    template<typename t_in>
    using is_seqs_v_t = t<is_seqs_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqs_stu {
        using type = is_seqs_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqs_bind = bind<tem<is_seqs_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqs_t_cpt = is_t_cpt<t_in> && is_seqs_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqs_t_boo{ is_seqs_t_cpt<t_in> };
    template<typename t_in>
    using is_seqs_t_v = v<is_seqs_t_boo<t_in>>;
    template<typename t_in>
    using is_seqs_t_v_t = t<is_seqs_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqs_t_stu {
        using type = is_seqs_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqs_t_bind = bind<tem<is_seqs_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqs_v_cpt = is_v_cpt<t_in> && is_seqs_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqs_v_boo{ is_seqs_v_cpt<t_in> };
    template<typename t_in>
    using is_seqs_v_v = v<is_seqs_v_boo<t_in>>;
    template<typename t_in>
    using is_seqs_v_v_t = t<is_seqs_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqs_v_stu {
        using type = is_seqs_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqs_v_bind = bind<tem<is_seqs_v_stu>>;
}

namespace lgo {
    template<typename t_in>
    concept is_seqs_impl_cpt = is_arrs_cpt<t_in>
    || is_strs_impl_cpt<t_in> || is_mems_cpt<t_in>;
}

#endif