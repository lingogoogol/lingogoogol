#ifndef LGO_BASIC_SEQ_MAIN_MD_OPT_LEN_CPT
#define LGO_BASIC_SEQ_MAIN_MD_OPT_LEN_CPT

#include "alias.hpp"

#include "../../../../../type/constraint/bind.hpp"
#include "../../../../../type/tem/.hpp"

LGO_EXPORT namespace lgo {
    using is_seqmd_len_bind = bind<is_same_bind, bind_arg<U"0">, seqmd_len>;
    using is_seqmd_len_t_bind = bind<is_same_bind, bind_arg<U"0">, seqmd_len_t>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqmd_len_v_cpt = is_v_cpt<t_in> && is_same_cpt<typename t_in::type, seqmd_len>;
    template<typename t_in>
    constexpr inline boo is_seqmd_len_v_boo{ is_seqmd_len_v_cpt<t_in> };
    template<typename t_in>
    using is_seqmd_len_v_v = v<is_seqmd_len_v_boo<t_in>>;
    template<typename t_in>
    using is_seqmd_len_v_v_t = t<is_seqmd_len_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqmd_len_v_stu {
        using type = is_seqmd_len_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqmd_len_v_bind = bind<tem<is_seqmd_len_v_stu>>;
}

#endif