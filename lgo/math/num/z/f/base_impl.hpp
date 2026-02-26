#ifndef LGO_MATH_NUM_Z_F_BASE_IMPL
#define LGO_MATH_NUM_Z_F_BASE_IMPL

#include "base_def.hpp"

namespace lgo {
    template<is_intp_cpt t_type, is_intp_v_cpt t_len>
    constexpr auto numzf<t_type, t_len>::len() -> intpc {
        return t_len::val;
    }
    
    template<is_intp_cpt t_type, is_intp_v_cpt t_len>
    constexpr auto numzf<t_type, t_len>::bits() -> intpc {
        return len() * t_type::bits;
    }
    
    template<is_intp_cpt t_type, is_intp_v_cpt t_len>
    constexpr auto numzf<t_type, t_len>::lit(intsc in) -> void {
        m_lit = in;
        return;
    }
    
    template<is_intp_cpt t_type, is_intp_v_cpt t_len>
    constexpr auto numzf<t_type, t_len>::lit() const -> intsc {
        return m_lit;
    }
    
    template<is_intp_cpt t_type, is_intp_v_cpt t_len>
    constexpr auto numzf<t_type, t_len>::big() const -> intsc {
        return lit() + len() - U"1"_intpc;
    }
    
    template<is_intp_cpt t_type, is_intp_v_cpt t_len>
    template<is_int_cpt t_in>
    constexpr numzf<t_type, t_len>::numzf(t_in in, intsc lit): numz{ in }, m_lit{ lit } {
        ctor(in);
        return;
    }
    
    template<is_intp_cpt t_type, is_intp_v_cpt t_len>
    template<is_numz_cpt t_in>
    constexpr numzf<t_type, t_len>::numzf(const t_in& in, intsc lit): m_lit{ lit } {
        ctor(in);
        return;
    }

    template<is_intp_cpt t_type, is_intp_v_cpt t_len>
    template<typename t_self>
    constexpr auto numzf<t_type, t_len>::operator[](this t_self&& self, intsc pos) -> decltype(auto) {
        return self.m_data[self.pos_to_index(pos)];
    }
}

#endif