#ifndef LGO_BASIC_SEQ_MAIN_MD_OPT_RESERVE_CPT
#define LGO_BASIC_SEQ_MAIN_MD_OPT_RESERVE_CPT

#include "decl.hpp"
#include "alias.hpp"

#include "../../../../../type/constraint/common.hpp"
#include "../../../../../type/tem/.hpp"

LGO_EXPORT namespace lgo {
    using is_seqmd_reserve_bind = bind<is_same_bind, bind_arg<U"0">, seqmd_reserve>;
    using is_seqmd_reserve_t_bind = bind<is_same_bind, bind_arg<U"0">, seqmd_reserve_t>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqmd_reserve_v_cpt = is_v_cpt<t_in> && is_same_cpt<typename t_in::type, seqmd_reserve>;
    template<typename t_in>
    constexpr inline boo is_seqmd_reserve_v_boo{ is_seqmd_reserve_v_cpt<t_in> };
    template<typename t_in>
    using is_seqmd_reserve_v_v = v<is_seqmd_reserve_v_boo<t_in>>;
    template<typename t_in>
    using is_seqmd_reserve_v_v_t = t<is_seqmd_reserve_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqmd_reserve_v_stu {
        using type = is_seqmd_reserve_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqmd_reserve_v_bind = bind<tem<is_seqmd_reserve_v_stu>>;
}

#endif