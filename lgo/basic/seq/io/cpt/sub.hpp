#ifndef LGO_BASIC_SEQ_IO_CPT_SUB
#define LGO_BASIC_SEQ_IO_CPT_SUB

#include "../../str/cpt/sub_cpt.hpp"
#include "../../mem/cpt/sub_cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqiosub_cpt = is_strsub_cpt<t_in> || is_memsub_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_seqiosub_boo{ is_seqiosub_cpt<t_in> };
    template<typename t_in>
    using is_seqiosub_v = v<is_seqiosub_boo<t_in>>;
    template<typename t_in>
    using is_seqiosub_v_t = t<is_seqiosub_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqiosub_stu {
        using type = is_seqiosub_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqiosub_bind = bind<tem<is_seqiosub_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqiosub_t_cpt = is_t_cpt<t_in> && is_seqiosub_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqiosub_t_boo{ is_seqiosub_t_cpt<t_in> };
    template<typename t_in>
    using is_seqiosub_t_v = v<is_seqiosub_t_boo<t_in>>;
    template<typename t_in>
    using is_seqiosub_t_v_t = t<is_seqiosub_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqiosub_t_stu {
        using type = is_seqiosub_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqiosub_t_bind = bind<tem<is_seqiosub_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqiosub_v_cpt = is_v_cpt<t_in> && is_seqiosub_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqiosub_v_boo{ is_seqiosub_v_cpt<t_in> };
    template<typename t_in>
    using is_seqiosub_v_v = v<is_seqiosub_v_boo<t_in>>;
    template<typename t_in>
    using is_seqiosub_v_v_t = t<is_seqiosub_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqiosub_v_stu {
        using type = is_seqiosub_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqiosub_v_bind = bind<tem<is_seqiosub_v_stu>>;
}

#endif