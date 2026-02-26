#ifndef LGO_IO_CMD_CPT
#define LGO_IO_CMD_CPT

#include "def.hpp"
#include "alias.hpp"

LGO_EXPORT namespace lgo {
    using is_cmd_type_bind = bind<is_same_bind, bind_arg<U"0">, cmd_type>;
    using is_cmd_type_t_bind = bind<is_same_bind, bind_arg<U"0">, cmd_type_t>;
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_cmd_type_v_cpt = is_v_cpt<t_in> && is_same_cpt<typename t_in::type, cmd_type>;
    template<typename t_in>
    constexpr inline boo is_cmd_type_v_boo{ is_cmd_type_v_cpt<t_in> };
    template<typename t_in>
    using is_cmd_type_v_v = v<is_cmd_type_v_boo<t_in>>;
    template<typename t_in>
    using is_cmd_type_v_v_t = t<is_cmd_type_v_v<t_in>>;
}

namespace lgo {
    template<typename t_in>
    struct is_cmd_type_v_stu {
        using type = is_cmd_type_v_v<t_in>;
    };
}

LGO_EXPORT namespace lgo {
    using is_cmd_type_v_bind = bind<tem<is_cmd_type_v_stu>>;
}

#endif