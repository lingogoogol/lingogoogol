#ifndef LGO_BASIC_TYPE_BOO_CPT_LITERAL
#define LGO_BASIC_TYPE_BOO_CPT_LITERAL

#include "../literal_str/basic.hpp"

LGO_EXPORT namespace lgo {
    template<basic_literal_str t_in>
    constexpr auto operator""_cpt() -> cpt {
        constexpr bool is_0{ t_in.m_data[0] == U'0' };
        constexpr bool is_1{ t_in.m_data[0] == U'1' };
        static_assert(is_0 || is_1);
        return is_1;
    }
}

#endif