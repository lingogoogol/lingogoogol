#ifndef LGO_BASIC_BIND_COND
#define LGO_BASIC_BIND_COND

#include "../type/tem/.hpp"

namespace lgo {
    template<is_arrmc_cpt...>
    struct bind_cond_stu {};

    template<is_arrmc_cpt t_in, is_arrmc_cpt... t_other>
    requires (t_in::template type<intpc::val_v<U"0">>::val.basic())
    struct bind_cond_stu<t_in, t_other...> {
        using type = typename t_in::template type<intpc::val_v<U"2">>::template tapply<t_in::template type<intpc::val_v<U"1">>>;
    };

    template<is_arrmc_cpt t_in, is_arrmc_cpt... t_other>
    requires (!t_in::template type<intpc::val_v<U"0">>::val.basic())
    struct bind_cond_stu<t_in, t_other...> {
        using type = bind_cond_stu<t_other...>::type;
    };

    template<>
    struct bind_cond_stu<> {};
}

LGO_EXPORT namespace lgo {
    template<is_arrmc_cpt... t_in>
    using bind_cond = bind_cond_stu<t_in...>::type;
}

#endif