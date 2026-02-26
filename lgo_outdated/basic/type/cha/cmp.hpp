#ifndef LGO_BASIC_TYPE_CHA_CMP
#define LGO_BASIC_TYPE_CHA_CMP

#include "def.hpp"

#include "../int/s/life.hpp"

namespace lgo {
    template<is_basic_cha_cpt t_type>
    template<is_cha_cpt t_in>
    constexpr auto cha<t_type>::operator<=>(t_in in) const -> intsc {
        if (m_data < in.m_data) {
            return intsc{ -1 };
        }
        else if (m_data > in.m_data) {
            return intsc{ 1 };
        }
        else {
            return intsc{ 0 };
        }
    }
}

#endif