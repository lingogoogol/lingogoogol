#ifndef LGO_BASIC_SEQ_MAIN_SUBC_OPT_LEN_CPT
#define LGO_BASIC_SEQ_MAIN_SUBC_OPT_LEN_CPT

#include "def.hpp"

namespace lgo {
    template<typename>
    struct is_seqsubc_len_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_seqsubc_len_stu<seqsubc_len<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqsubc_len_bind = bind<tem<is_seqsubc_len_stu>>;
    template<typename t_in>
    using is_seqsubc_len_v = is_seqsubc_len_bind::type<t_in>;
    template<typename t_in>
    using is_seqsubc_len_v_t = t<is_seqsubc_len_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_seqsubc_len_boo{ is_seqsubc_len_v<t_in>::val };
    template<typename t_in>
    concept is_seqsubc_len_cpt = is_seqsubc_len_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqsubc_len_t_cpt = is_t_cpt<t_in> && is_seqsubc_len_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqsubc_len_t_boo{ is_seqsubc_len_t_cpt<t_in> };
    template<typename t_in>
    using is_seqsubc_len_t_v = v<is_seqsubc_len_t_boo<t_in>>;
    template<typename t_in>
    using is_seqsubc_len_t_v_t = t<is_seqsubc_len_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqsubc_len_t_stu {
        using type = is_seqsubc_len_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqsubc_len_t_bind = bind<tem<is_seqsubc_len_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seqsubc_len_v_cpt = is_v_cpt<t_in> && is_seqsubc_len_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seqsubc_len_v_boo{ is_seqsubc_len_v_cpt<t_in> };
    template<typename t_in>
    using is_seqsubc_len_v_v = v<is_seqsubc_len_v_boo<t_in>>;
    template<typename t_in>
    using is_seqsubc_len_v_v_t = t<is_seqsubc_len_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seqsubc_len_v_stu {
        using type = is_seqsubc_len_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seqsubc_len_v_bind = bind<tem<is_seqsubc_len_v_stu>>;
}

#endif