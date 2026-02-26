#ifndef LGO_BASIC_TYPE_DEDUCE_COND
#define LGO_BASIC_TYPE_DEDUCE_COND

#include "../int/p/literal.hpp"
#include "../tem/v/.hpp"
#include "../boo/impl.hpp"
#include "../cha/basic_impl.hpp"
#include "../../seq/arr/mc/main/decl.hpp"
#include "../../seq/arr/mc/main/cpt.hpp"

namespace lgo {
    template<is_arrmc_cpt t_in, is_arrmc_cpt... t_other>
    struct tcond_stu {
        using type = tcond_stu<t_other...>::type;
    };

    template<is_arrmc_cpt t_in, is_arrmc_cpt... t_other>
    requires ((t_in::template type<intpc::val_v<U"0">>::val).cpt())
    struct tcond_stu<t_in, t_other...> {
        using type = t_in::template type<intpc::val_v<U"1">>;
    };
}

LGO_EXPORT namespace lgo {
    template<is_arrmc_cpt... t_in>
    using tcond = tcond_stu<t_in...>::type;
}

#endif