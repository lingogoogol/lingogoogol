#ifndef LGO_BASIC_TYPE_PTR_MAIN_CPT
#define LGO_BASIC_TYPE_PTR_MAIN_CPT

#include "../cpt/d.hpp"
#include "../s/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_ptr_cpt = is_ptrs_cpt<t_in> || is_ptrd_cpt<t_in>;
    template<typename t_in>
    constexpr inline boo is_ptr_boo{ is_ptr_cpt<t_in> };
    template<typename t_in>
    using is_ptr_v = v<is_ptr_boo<t_in>>;
    template<typename t_in>
    using is_ptr_v_t = t<is_ptr_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ptr_stu {
        using type = is_ptr_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ptr_bind = bind<tem<is_ptr_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_ptr_t_cpt = is_t_cpt<t_in> && is_ptr_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_ptr_t_boo{ is_ptr_t_cpt<t_in> };
    template<typename t_in>
    using is_ptr_t_v = v<is_ptr_t_boo<t_in>>;
    template<typename t_in>
    using is_ptr_t_v_t = t<is_ptr_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ptr_t_stu {
        using type = is_ptr_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ptr_t_bind = bind<tem<is_ptr_t_stu>>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_ptr_v_cpt = is_v_cpt<t_in> && is_ptr_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_ptr_v_boo{ is_ptr_v_cpt<t_in> };
    template<typename t_in>
    using is_ptr_v_v = v<is_ptr_v_boo<t_in>>;
    template<typename t_in>
    using is_ptr_v_v_t = t<is_ptr_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_ptr_v_stu {
        using type = is_ptr_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_ptr_v_bind = bind<tem<is_ptr_v_stu>>;
}

#endif