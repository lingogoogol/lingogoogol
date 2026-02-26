#ifndef LGO_BASIC_TYPE_DEDUCE_QUALIFIER_APPLY_BASIC
#define LGO_BASIC_TYPE_DEDUCE_QUALIFIER_APPLY_BASIC

#include "../constraint/qualifier.hpp"

namespace lgo {
    template<typename t_target, typename t_in>
    struct apply_qlr_basic_stu {
        using type = t_target;
    };

    template<typename t_target, typename t_in>
    requires is_qlr_cpt<t_in>
    struct apply_qlr_basic_stu<t_target, t_in> {
        using type = t_target&;
    };

    template<typename t_target, typename t_in>
    struct apply_qrr_basic_stu {
        using type = t_target;
    };

    template<typename t_target, typename t_in>
    requires is_qrr_cpt<t_in>
    struct apply_qrr_basic_stu<t_target, t_in> {
        using type = t_target&&;
    };
}

namespace lgo {
    template<typename t_target, typename t_in>
    using apply_qlr_basic = apply_qlr_basic_stu<t_target, t_in>::type;
    template<typename t_target, typename t_in>
    using apply_qrr_basic = apply_qrr_basic_stu<t_target, t_in>::type;
    template<typename t_target, typename t_in>
    using apply_qr_basic = apply_qrr_basic<apply_qlr_basic<t_target, t_in>, t_in>;
}

#endif