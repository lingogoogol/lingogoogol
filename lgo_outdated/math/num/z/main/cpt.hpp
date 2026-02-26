#ifndef LGO_MATH_NUM_Z_MAIN_CPT
#define LGO_MATH_NUM_Z_MAIN_CPT

#include "../s/cpt.hpp"
#include "../f/cpt.hpp"
#include "../d/cpt.hpp"

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numz_cpt = is_numzs_cpt<t_in> || is_numzf_cpt<t_in> || is_numzd_cpt<t_in>;
	template<typename t_in>
	constexpr inline boo is_numz_boo{ is_numz_cpt<t_in> };
	template<typename t_in>
	using is_numz_v = v<is_numz_boo<t_in>>;
	template<typename t_in>
	using is_numz_v_t = t<is_numz_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numz_stu {
        using type = is_numz_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
	using is_numz_bind = bind<tem<is_numz_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numz_t_cpt = is_t_cpt<t_in> && is_numz_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numz_t_boo{ is_numz_t_cpt<t_in> };
    template<typename t_in>
    using is_numz_t_v = v<is_numz_t_boo<t_in>>;
    template<typename t_in>
    using is_numz_t_v_t = t<is_numz_t_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numz_t_stu {
        using type = is_numz_t_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numz_t_bind = bind<tem<is_numz_t_stu>>;
}

LGO_EXPORT namespace lgo {
	template<typename t_in>
	concept is_numz_v_cpt = is_v_cpt<t_in> && is_numz_cpt<typename t_in::type>;
    template<typename t_in>
    constexpr inline boo is_numz_v_boo{ is_numz_v_cpt<t_in> };
    template<typename t_in>
    using is_numz_v_v = v<is_numz_v_boo<t_in>>;
    template<typename t_in>
    using is_numz_v_v_t = t<is_numz_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_numz_v_stu {
        using type = is_numz_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_numz_v_bind = bind<tem<is_numz_v_stu>>;
}

namespace lgo {
    template<typename t_in>
    concept is_numz_impl_cpt = is_numzs_impl_cpt<t_in> || is_numzf_impl_cpt<t_in> || is_numzd_cpt<t_in>;
}

#endif