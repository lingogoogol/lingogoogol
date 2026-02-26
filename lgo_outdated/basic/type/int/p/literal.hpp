#ifndef LGO_BASIC_TYPE_INT_P_LITERAL
#define LGO_BASIC_TYPE_INT_P_LITERAL

#include "impl.hpp"
#include "literal_decl.hpp"

#include "../u/impl.hpp"

LGO_EXPORT namespace lgo::inline literal {
    template<basic_literal_str t_in>
    constexpr auto operator""_intp8() -> intp8 {
        return intp8::make<t_in>();
    }

    template<basic_literal_str t_in>
    constexpr auto operator""_intp16() -> intp16 {
        return intp16::make<t_in>();
    }

    template<basic_literal_str t_in>
    constexpr auto operator""_intp32() -> intp32 {
        return intp32::make<t_in>();
    }

    template<basic_literal_str t_in>
    constexpr auto operator""_intp64() -> intp64 {
        return intp64::make<t_in>();
    }

    template<basic_literal_str t_in>
    constexpr auto operator""_intpc() -> intpc {
        return intpc::make<t_in>();
    }
}

#endif