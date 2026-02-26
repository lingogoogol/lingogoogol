#ifndef LGO_BASIC_FUN_BASE
#define LGO_BASIC_FUN_BASE

#include "type.hpp"

namespace lgo {
    template<typename t_type>
    struct fun_base {
        using type = empty;
    };

    template<typename t_type>
    requires requires(t_type fun) { std::function{ fun }; }
    struct fun_base<t_type> {
        using type = fun_type<t_type>;
    };
}

#endif