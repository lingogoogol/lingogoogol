#ifndef LGO_BASIC_SEQ_STR_MS_CPT
#define LGO_BASIC_SEQ_STR_MS_CPT

#include "alias_decl.hpp"
#include "decl.hpp"

#include "../../../type/tem/t/cpt.hpp"
#include "../../../type/tem/v/cpt.hpp"

namespace lgo {
    template<typename>
    struct is_strms8_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<typename... t_in>
    struct is_strms8_cpt_stu<strms8<t_in...>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_strms8_cpt = is_strms8_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_strms8_t_cpt = is_t_cpt<t_in> && is_strms8_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_strms8_v_cpt = is_v_cpt<t_in> && is_strms8_cpt<typename t_in::type>;
}

namespace lgo {
    template<typename>
    struct is_strms16_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<typename... t_in>
    struct is_strms16_cpt_stu<strms16<t_in...>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_strms16_cpt = is_strms16_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_strms16_t_cpt = is_t_cpt<t_in> && is_strms16_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_strms16_v_cpt = is_v_cpt<t_in> && is_strms16_cpt<typename t_in::type>;
}

namespace lgo {
    template<typename>
    struct is_strms32_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<typename... t_in>
    struct is_strms32_cpt_stu<strms32<t_in...>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_strms32_cpt = is_strms32_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_strms32_t_cpt = is_t_cpt<t_in> && is_strms32_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_strms32_v_cpt = is_v_cpt<t_in> && is_strms32_cpt<typename t_in::type>;
}

namespace lgo {
    template<typename>
    struct is_strmsc_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<typename... t_in>
    struct is_strmsc_cpt_stu<strmsc<t_in...>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_strmsc_cpt = is_strmsc_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_strmsc_t_cpt = is_t_cpt<t_in> && is_strmsc_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_strmsc_v_cpt = is_v_cpt<t_in> && is_strmsc_cpt<typename t_in::type>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_strms_cpt = is_strms8_cpt<t_in>
    || is_strms16_cpt<t_in> || is_strms32_cpt<t_in> || is_strmsc_cpt<t_in>;
    template<typename t_in>
    concept is_strms_t_cpt = is_t_cpt<t_in> && is_strms_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_strms_v_cpt = is_v_cpt<t_in> && is_strms_cpt<typename t_in::type>;
}

namespace lgo {
    template<typename>
    struct is_strms_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<typename... t_in>
    struct is_strms_cpt_stu<strms<t_in...>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

namespace lgo {
    template<typename t_in>
    concept is_strms_impl_cpt = is_strms_cpt<t_in> || is_strms_cpt_stu<t_in>::val;
}

#endif