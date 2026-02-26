#ifndef LGO_BASIC_TYPE_FUN_PACK_SIZE
#define LGO_BASIC_TYPE_FUN_PACK_SIZE

#include "placeholder/.hpp"

#include "../int/p/.hpp"

namespace lgo {
	constexpr auto pack_size_fun() -> intpc {
		return U"0"_intpc;
	}

    template<is_placeholder_cpt t_in, is_placeholder_cpt... t_other>
    constexpr auto pack_size_fun(t_in, t_other... other) -> intpc {
        return pack_size_fun(other...) + U"1"_intpc;
    }
}

LGO_EXPORT namespace lgo {
    template<typename... t_in>
    constexpr inline intpc pack_size{ pack_size_fun(placeholder<t_in>{}...) };
}

#endif