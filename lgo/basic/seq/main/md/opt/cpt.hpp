#ifndef LGO_BASIC_SEQ_MAIN_MD_OPT_CPT
#define LGO_BASIC_SEQ_MAIN_MD_OPT_CPT

#include "len/cpt.hpp"
#include "reserve/cpt.hpp"

#include "../../main/opt/data/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqmd_opt_cpt = is_same_cpt<t_in, seqmd_reserve> || is_same_cpt<t_in, seqmd_len> || is_seq_data_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_seqmd_opt_boo{ is_seqmd_opt_cpt<t_in> };
    template<typename t_in>
    using is_seqmd_opt_v = v<is_seqmd_opt_boo<t_in>>;
    template<typename t_in>
    using is_seqmd_opt_v_t = t<is_seqmd_opt_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqmd_opt_stu {
        using type = is_seqmd_opt_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqmd_opt_bind = bind<tem<is_seqmd_opt_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqmd_opt_t_cpt = is_t_cpt<t_in> && is_seqmd_opt_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqmd_opt_t_boo{ is_seqmd_opt_t_cpt<t_in> };
    template<typename t_in>
    using is_seqmd_opt_t_v = v<is_seqmd_opt_t_boo<t_in>>;
    template<typename t_in>
    using is_seqmd_opt_t_v_t = t<is_seqmd_opt_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqmd_opt_t_stu {
        using type = is_seqmd_opt_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqmd_opt_t_bind = bind<tem<is_seqmd_opt_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqmd_opt_v_cpt = is_v_cpt<t_in> && is_seqmd_opt_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqmd_opt_v_boo{ is_seqmd_opt_v_cpt<t_in> };
    template<typename t_in>
    using is_seqmd_opt_v_v = v<is_seqmd_opt_v_boo<t_in>>;
    template<typename t_in>
    using is_seqmd_opt_v_v_t = t<is_seqmd_opt_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqmd_opt_v_stu {
        using type = is_seqmd_opt_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqmd_opt_v_bind = bind<tem<is_seqmd_opt_v_stu>>;
}

#endif