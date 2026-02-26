#ifndef LGO_BASIC_SEQ_MAIN_SUBC_DEF
#define LGO_BASIC_SEQ_MAIN_SUBC_DEF

#include "opt/.hpp"

#include "../../../type/tem/t/.hpp"
#include "../../../type/ptr/m/.hpp"
#include "../../../type/fun/placeholder/.hpp"
#include "../../../bind/cond.hpp"

namespace lgo {
    template<typename t_type, is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
    class seqsubc: public seqc {
    protected:
        template<typename u_in>
        constexpr seqsubc(u_in& in, placeholder<intpc::val_v<U"0">>)
        requires is_seqm_cpt<rm_qcv<u_in>>;
        template<typename u_in>
        constexpr seqsubc(u_in& in, placeholder<intpc::val_v<U"0">>)
        requires is_seqsub_cpt<rm_qcv<u_in>>;
    public:
        template<is_intp_v_cpt u_pos>
        using type_ind = typename t_type::template type_ind<u_pos>;
    public:
        constexpr seqsubc() = delete;

        template<is_seqc_cpt u_in>
        constexpr auto operator=(const u_in& in) -> seqsubc& requires (!is_qc_cpt<t_type>);
        constexpr auto operator=(const seqsubc& in) -> seqsubc& requires (!is_qc_cpt<t_type>);

        constexpr auto len() const -> intpc;

        template<is_intp_v_cpt u_pos>
        constexpr auto ind() -> decltype(auto);
        template<is_intp_v_cpt u_pos>
        constexpr auto ind() const -> const type_ind<u_pos>&;
    private:
        template<typename u_self, is_seqsubc_opt_cpt... u_opt>
        struct submc_ret_stu {
            using opt = arrmc<u_opt...>;
            using pos = bind_cond<arrmc<typename opt::template tcontain_v<is_seqsubc_pos_bind>, typename opt::tind_bind
            , arrmc<typename opt::template tfind<is_seqsubc_pos_bind>::template tind<intpc::val_v<U"0">>>>
            , arrmc<boo::val_v<U"1">, iden_bind, arrmc<seqsubc_pos<intpc::val_v<U"0">>>>>;
            using len = bind_cond<arrmc<typename opt::template tcontain_v<is_seqsubc_len_bind>, typename opt::tind_bind
            , arrmc<typename opt::template tfind<is_seqsubc_len_bind>::template tind<intpc::val_v<U"0">>>>
            , arrmc<boo::val_v<U"1">, iden_bind, arrmc<seqsubc_len<intpc::val_v<U"0">>>>>;
            constexpr static intpc new_pos{ t_pos::val + pos::val };
            constexpr static intpc new_len{ len::val };
            using type = seqsubc<apply_qcv<t_type, u_self>, v<new_pos>, v<new_len>>;
        };
    public:
        template<typename... t_opt, typename t_self>
        constexpr auto subcc(this t_self& self) -> typename submc_ret_stu<const t_self, t_opt...>::type;
        template<typename... t_opt, typename t_self>
        constexpr auto submc(this t_self& self) -> typename submc_ret_stu<t_self, t_opt...>::type;
    protected:
        ptrs<t<t_type>> m_data{};
    };
}

#endif