#ifndef LGO_BASIC_SEQ_MAIN_SUBC_IMPL
#define LGO_BASIC_SEQ_MAIN_SUBC_IMPL

#include "def.hpp"

namespace lgo {
    template<typename t_type, is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
    template<typename u_in>
    constexpr seqsubc<t_type, t_pos, t_len>::seqsubc(u_in& in, placeholder<intpc::val_v<U"0">>)
    requires is_seqm_cpt<rm_qcv<u_in>>: m_data{ &in } {}

    template<typename t_type, is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
    template<typename u_in>
    constexpr seqsubc<t_type, t_pos, t_len>::seqsubc(u_in& in, placeholder<intpc::val_v<U"0">>)
    requires is_seqsub_cpt<rm_qcv<u_in>>: m_data{ in.m_data } {}

    template<typename t_type, is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
    template<is_seqc_cpt u_in>
    constexpr auto seqsubc<t_type, t_pos, t_len>::operator=(const u_in& in)
    -> seqsubc& requires (!is_qc_cpt<t_type>) {
        if constexpr (len() != U"0"_intpc) {
            ind<intpc::val_v<U"0">>() = in.template ind<intpc::val_v<U"0">>();
            submc<intpc::val_v<U"1">>() = in.template subcc<intpc::val_v<U"0">>();
        }
        return *this;
    }

    template<typename t_type, is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
    constexpr auto seqsubc<t_type, t_pos, t_len>::operator=(const seqsubc& in) -> seqsubc& requires (!is_qc_cpt<t_type>) {
        return operator=<seqsubc>(in);
    }

    template<typename t_type, is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
    constexpr auto seqsubc<t_type, t_pos, t_len>::len() const -> intpc {
        return t_len::val;
    }

    template<typename t_type, is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
    template<is_intp_v_cpt u_pos>
    constexpr auto seqsubc<t_type, t_pos, t_len>::ind() -> decltype(auto) {
        constexpr static intpc actual_ind{ t_pos::val + u_pos::val };
        return m_data.ref().ind<v<actual_ind>>();
    }

    template<typename t_type, is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
    template<is_intp_v_cpt u_pos>
    constexpr auto seqsubc<t_type, t_pos, t_len>::ind() const -> const type_ind<u_pos>& {
        constexpr static intpc actual_ind{ t_pos::val + u_pos::val };
        return m_data.ref().ind<v<actual_ind>>();
    }

    template<typename t_type, is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
    template<typename... t_opt, typename t_self>
    constexpr auto seqsubc<t_type, t_pos, t_len>::subcc(this t_self& self) -> typename submc_ret_stu<const t_self, t_opt...>::type {
        return typename submc_ret_stu<const t_self, t_opt...>::type{ self };
    }

    template<typename t_type, is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
    template<typename... t_opt, typename t_self>
    constexpr auto seqsubc<t_type, t_pos, t_len>::submc(this t_self& self) -> typename submc_ret_stu<t_self, t_opt...>::type {
        return typename submc_ret_stu<t_self, t_opt...>::type{ self };
    }
}

#endif