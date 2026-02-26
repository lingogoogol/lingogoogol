#ifndef LGO_BASIC_BIND_QUALIFIER_ADD
#define LGO_BASIC_BIND_QUALIFIER_ADD

#include "../type/deduce/qualifier_add.hpp"

namespace lgo {
    template<typename t_in>
    struct add_qc_bind_stu {
        using type = add_qc<t_in>;
    };

    template<typename t_in>
    struct add_qv_bind_stu {
        using type = add_qv<t_in>;
    };

    template<typename t_in>
    struct add_qp_bind_stu {
        using type = add_qp<t_in>;
    };

    template<typename t_in>
    struct add_qlr_bind_stu {
        using type = add_qlr<t_in>;
    };

    template<typename t_in>
    struct add_qrr_bind_stu {
        using type = add_qrr<t_in>;
    };

    template<typename t_in>
    struct add_qcv_bind_stu {
        using type = add_qcv<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using add_qc_bind = bind<tem<add_qc_bind_stu>>;
    using add_qv_bind = bind<tem<add_qv_bind_stu>>;

    using add_qp_bind = bind<tem<add_qp_bind_stu>>;
    using add_qlr_bind = bind<tem<add_qlr_bind_stu>>;
    using add_qrr_bind = bind<tem<add_qrr_bind_stu>>;
    
    using add_qcv_bind = bind<tem<add_qcv_bind_stu>>;
}

#endif