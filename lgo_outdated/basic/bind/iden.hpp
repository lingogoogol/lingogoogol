#ifndef LGO_BASIC_BIND_IDEN
#define LGO_BASIC_BIND_IDEN

#include "../type/tem/bind/decl.hpp"
#include "../type/tem/tem/def.hpp"
#include "../seq/arr/mc/main/cpt.hpp"

namespace lgo {
    template<typename t_in>
    struct iden_stu {
        using type = t_in;
    };
}

LGO_EXPORT namespace lgo {
    using iden_bind = bind<tem<iden_stu>>;
}

namespace lgo {
    template<is_arrmc_cpt, is_bind_cpt t_bind, typename... t_in>
    struct suspend_primary {
        using type = t_bind::template type<t_in...>;
    };

    template<is_bind_cpt t_bind, typename... t_in>
    struct suspend_outer {
        template<typename... u_suspender>
        struct suspend_stu {
            using type = suspend_primary<arrmc<u_suspender...>, t_bind, t_in...>::type;
        };
    };
}

LGO_EXPORT namespace lgo {
    template<is_bind_cpt t_bind, typename... t_in>
    using suspend_bind = bind<tem<suspend_outer<t_bind, t_in...>::template suspend_stu>>;
}

#endif