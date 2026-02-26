#ifndef LGO_BASIC_SEQ_MAIN_MS_NONEMPTY_IMPL
#define LGO_BASIC_SEQ_MAIN_MS_NONEMPTY_IMPL

#include "nonempty_def.hpp"

namespace lgo {
    template<typename t_type, is_intp_v_cpt t_len>
    constexpr auto seqms<t_type, t_len>::size(size_stack) -> intpc {
        return intpc{ sizeof(type) } * len();
    }
    
    template<typename t_type, is_intp_v_cpt t_len>
    constexpr auto seqms<t_type, t_len>::size(size_content) -> intpc
    requires requires { type::size(size_content{}); } {
        return type::size(size_content{}) * len();
    }
    
    template<typename t_type, is_intp_v_cpt t_len>
    constexpr auto seqms<t_type, t_len>::size(size_content) -> intpc
    requires (!requires { type::size(size_content{}); }) {
        intpc out{ U"0"_intpc };
        for (intpc i{ U"0"_intpc }; (i < len()).basic(); ++i) {
            out += (*this)[i].size(size_content{});
        }
        return out;
    }

    template<typename t_type, is_intp_v_cpt t_len>
    constexpr auto seqms<t_type, t_len>::len() -> intpc {
        return t_len::val;
    }

    template<typename t_type, is_intp_v_cpt t_len>
    template<typename... t_in>
    constexpr seqms<t_type, t_len>::seqms(t_in... in): m_data{ in... } {}

    template<typename t_type, is_intp_v_cpt t_len>
    template<is_seqs_cpt t_self, is_seq_cpt t_in>
    constexpr auto seqms<t_type, t_len>::operator=(this t_self& self, const t_in& in) -> t_self& {
        in;
        return self;
    }
    
    template<typename t_type, is_intp_v_cpt t_len>
    template<typename t_self>
    constexpr auto seqms<t_type, t_len>::operator[](this t_self& self, intpc pos) -> decltype(auto) {
        if ((pos >= self.len()).basic()) {
            throw e_other{ U""_strmdc };
        }
        return self.m_data[pos.basic()];
    }
    
    template<typename t_type, is_intp_v_cpt t_len>
    template<typename t_self, is_intp_v_cpt t_pos>
    constexpr auto seqms<t_type, t_len>::ind(this t_self& self) -> decltype(auto) {
        return self[t_pos::val];
    }

    template<typename t_type, is_intp_v_cpt t_len>
    template<typename t_self>
	constexpr auto seqms<t_type, t_len>::data(this t_self&& self) -> auto {
		return ptrs{ self.m_data };
	}
}

#endif