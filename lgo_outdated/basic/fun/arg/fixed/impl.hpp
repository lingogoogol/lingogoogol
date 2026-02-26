#ifndef LGO_BASIC_FUN_ARG_FIXED_IMPL
#define LGO_BASIC_FUN_ARG_FIXED_IMPL

#include "def.hpp"

namespace lgo {
    template<typename t_type>
    constexpr fun_fixed<t_type>::fun_fixed(t_type&& in): m_data{ &::lgo::f<t_type>(in) } {}

    template<typename t_type>
    constexpr auto fun_fixed<t_type>::data() -> t_type&& {
        return m_data.r().ref();
    }
}

#endif