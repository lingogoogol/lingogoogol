#ifndef LGO_BASIC_SEQ_MAIN_SUBS_IMPL
#define LGO_BASIC_SEQ_MAIN_SUBS_IMPL

#include "def.hpp"

namespace lgo {
    template<typename t_type>
    template<typename u_in>
    constexpr seqsubs<t_type>::seqsubs(u_in& in, placeholder<intpc::val_v<U"0">>)
    : seqsubs{ in, U"0"_intpc, placeholder<intpc::val_v<U"0">>{} } {}

    template<typename t_type>
    template<typename u_in>
    constexpr seqsubs<t_type>::seqsubs(u_in& in, intpc pos, placeholder<intpc::val_v<U"0">>)
    : seqsubs{ in, pos, in.len() - pos, placeholder<intpc::val_v<U"0">>{} } {}

    template<typename t_type>
    template<typename u_in>
    constexpr seqsubs<t_type>::seqsubs(u_in& in, intpc pos, intpc len, placeholder<intpc::val_v<U"0">>)
    requires is_seqm_cpt<rm_qcv<u_in>>: m_data{ &in }, m_pos{ pos }, m_len{ len } {
        if ((pos + len > in.len()).basic()) {
            throw e_other{ U""_strmdc };
        }
        return;
    }

    template<typename t_type>
    template<typename u_in>
    constexpr seqsubs<t_type>::seqsubs(u_in& in, intpc pos, intpc len, placeholder<intpc::val_v<U"0">>)
    requires is_seqsub_cpt<rm_qcv<u_in>>
    : m_data{ in.m_data }, m_pos{ in.m_pos + pos }, m_len{ len } {
        if ((pos + len > in.len()).basic()) {
            throw e_other{ U""_strmdc };
        }
        return;
    }

    template<typename t_type>
    template<typename t_self, is_seq_cpt u_in>
    constexpr auto seqsubs<t_type>::operator=(this t_self&& self, const u_in& in) -> t_self&& requires (!is_qc_cpt<type_actual>) {
        if ((self.len() != in.len()).basic()) {
            throw e_other{ U""_strmdc };
        }
        for (intpc i{ U"0"_intpc }; (i < self.len()).basic(); ++i) {
            self[i] = in[i];
        }
        return self.f<t_self>();
    }

    template<typename t_type>
    constexpr seqsubs<t_type>::operator ptrm<seqsubs_access::type_qualified<t_type>>() const {
        return &(*this)[U"0"_intpc];
    }

    template<typename t_type>
    constexpr auto seqsubs<t_type>::len() const -> intpc {
        return m_len;
    }

    template<typename t_type>
    template<typename t_self>
    constexpr auto seqsubs<t_type>::operator[](this t_self&& self, intpc pos) -> decltype(auto) {
        return (*self.m_data)[self.m_pos + pos];
    }

    template<typename t_type>
    constexpr auto seqsubs<t_type>::operator[](intpc pos) const -> const type& {
        return (*m_data)[m_pos + pos];
    }
}

#endif