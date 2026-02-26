#ifndef LGO_BASIC_TYPE_LITERAL_STR_IMPL
#define LGO_BASIC_TYPE_LITERAL_STR_IMPL

#include "def.hpp"

#include "../int/p/life.hpp"
#include "../cha/impl.hpp"

namespace lgo {
    template<is_intp_v_cpt t_len>
    constexpr auto literal_str<t_len>::len() -> intpc {
        return t_len::val;
    }

    template<is_intp_v_cpt t_len>
    template<basic_intpc u_len>
    constexpr literal_str<t_len>::literal_str(const basic_chac (&in)[u_len]) {
        for (intpc i{ U"0"_intpc }; i < len(); ++i) {
            m_data[i.basic()] = in[i.basic()];
        }  
        return;
    }

    template<is_intp_v_cpt t_len>
    template<typename t_self>
    constexpr auto literal_str<t_len>::operator[](this t_self self, intpc index) -> chac {
        if (index >= len()) {
            throw e_other{ U""_strmdc };
        }
        return ::lgo::sc<chac>(self.m_data[index.basic()]);
    }
}

#endif