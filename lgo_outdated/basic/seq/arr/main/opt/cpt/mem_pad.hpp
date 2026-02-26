#ifndef LGO_BASIC_SEQ_ARR_MAIN_OPT_CPT_MEM_PAD
#define LGO_BASIC_SEQ_ARR_MAIN_OPT_CPT_MEM_PAD

#include "../for_all/cpt.hpp"
#include "../../../../main/main/opt/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arr_opt_mem_pad_cpt = is_arr_for_all_cpt<t_in>
    || is_same_cpt<t_in, seq_plain> || is_same_cpt<t_in, seq_inverse>;
    template<typename t_in>
    constexpr inline boo is_arr_opt_mem_pad_boo{ is_arr_opt_mem_pad_cpt<t_in> };
    template<typename t_in>
    using is_arr_opt_mem_pad_v = v<is_arr_opt_mem_pad_boo<t_in>>;
    template<typename t_in>
    using is_arr_opt_mem_pad_v_t = t<is_arr_opt_mem_pad_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arr_opt_mem_pad_stu {
        using type = is_arr_opt_mem_pad_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arr_opt_mem_pad_bind = bind<tem<is_arr_opt_mem_pad_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arr_opt_mem_pad_t_cpt = is_t_cpt<t_in> && is_arr_opt_mem_pad_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arr_opt_mem_pad_t_boo{ is_arr_opt_mem_pad_t_cpt<t_in> };
    template<typename t_in>
    using is_arr_opt_mem_pad_t_v = v<is_arr_opt_mem_pad_t_boo<t_in>>;
    template<typename t_in>
    using is_arr_opt_mem_pad_t_v_t = t<is_arr_opt_mem_pad_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arr_opt_mem_pad_t_stu {
        using type = is_arr_opt_mem_pad_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arr_opt_mem_pad_t_bind = bind<tem<is_arr_opt_mem_pad_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_arr_opt_mem_pad_v_cpt = is_v_cpt<t_in> && is_arr_opt_mem_pad_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_arr_opt_mem_pad_v_boo{ is_arr_opt_mem_pad_v_cpt<t_in> };
    template<typename t_in>
    using is_arr_opt_mem_pad_v_v = v<is_arr_opt_mem_pad_v_boo<t_in>>;
    template<typename t_in>
    using is_arr_opt_mem_pad_v_v_t = t<is_arr_opt_mem_pad_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_arr_opt_mem_pad_v_stu {
        using type = is_arr_opt_mem_pad_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_arr_opt_mem_pad_v_bind = bind<tem<is_arr_opt_mem_pad_v_stu>>;
}

#endif