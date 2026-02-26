#ifndef LGO_BASIC_TYPE_INT_N_LITERAL
#define LGO_BASIC_TYPE_INT_N_LITERAL

#include "impl.hpp"
#include "literal_decl.hpp"

#include "../u/impl.hpp"

LGO_EXPORT namespace lgo::inline literal {
    template<basic_literal_str t_in>
    constexpr auto operator""_intn8() -> intn8 {
        return intn8::make<t_in>();
    }

    template<basic_literal_str t_in>
    constexpr auto operator""_intn16() -> intn16 {
        return intn16::make<t_in>();
    }

    template<basic_literal_str t_in>
    constexpr auto operator""_intn32() -> intn32 {
        return intn32::make<t_in>();
    }

    template<basic_literal_str t_in>
    constexpr auto operator""_intn64() -> intn64 {
        return intn64::make<t_in>();
    }

    template<basic_literal_str t_in>
    constexpr auto operator""_intnc() -> intnc {
        return intnc::make<t_in>();
    }
}

#endif