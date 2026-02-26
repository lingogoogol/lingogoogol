#ifndef LGO_BASIC_TYPE_BOO_LITERAL
#define LGO_BASIC_TYPE_BOO_LITERAL

#include "def.hpp"

#include "../literal_str/basic.hpp"

LGO_EXPORT namespace lgo::inline literal {
    template<basic_literal_str t_in>
    constexpr auto operator""_boo() -> boo {
        return boo::val<t_in>;
    }
}

#endif