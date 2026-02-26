#ifndef LGO_BASIC_SEQ_MAIN_MAIN_OPT_DATA_CPT
#define LGO_BASIC_SEQ_MAIN_MAIN_OPT_DATA_CPT

#include "decl.hpp"

#include "../../../../../type/fun/cast_impl.hpp"
#include "../../../../../type/boo/.hpp"
#include "../../../../../type/tem/.hpp"

namespace lgo {
    template<typename>
    struct is_seq_data_stu {
        using type = boo::val_v<U"0">;
    };

    template<typename... t_in>
    struct is_seq_data_stu<seq_data<t_in...>> {
        using type = boo::val_v<U"1">;
    };
}

LGO_EXPORT namespace lgo {
    using is_seq_data_bind = bind<tem<is_seq_data_stu>>;
    template<typename t_in>
    using is_seq_data_v = is_seq_data_bind::type<t_in>;
    template<typename t_in>
    using is_seq_data_v_t = t<is_seq_data_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_seq_data_boo{ is_seq_data_v<t_in>::val };
    template<typename t_in>
    concept is_seq_data_cpt = is_seq_data_boo<t_in>.cpt();
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seq_data_t_cpt = is_t_cpt<t_in> && is_seq_data_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seq_data_t_boo{ is_seq_data_t_cpt<t_in> };
    template<typename t_in>
    using is_seq_data_t_v = v<is_seq_data_t_boo<t_in>>;
    template<typename t_in>
    using is_seq_data_t_v_t = t<is_seq_data_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seq_data_t_stu {
        using type = is_seq_data_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seq_data_t_bind = bind<tem<is_seq_data_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_seq_data_v_cpt = is_v_cpt<t_in> && is_seq_data_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_seq_data_v_boo{ is_seq_data_v_cpt<t_in>::val };
    template<typename t_in>
    using is_seq_data_v_v = v<is_seq_data_v_boo<t_in>>;
    template<typename t_in>
    using is_seq_data_v_v_t = t<is_seq_data_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_seq_data_v_stu {
        using type = is_seq_data_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_seq_data_v_bind = bind<tem<is_seq_data_v_stu>>;
}

#endif