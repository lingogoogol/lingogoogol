#ifndef LGO_BASIC_SEQ_MAIN_SUBS_DEF
#define LGO_BASIC_SEQ_MAIN_SUBS_DEF

#include "../s/def.hpp"
#include "../../../type/fun/placeholder/.hpp"

namespace lgo {
    template<typename t_type>
    class seqsubs;

    struct seqsubs_access {
        template<typename t_type>
        using type_qualified = typename seqsubs<t_type>::type_qualified;
    };

    template<typename t_type>
    class seqsubs: public seqs {
    protected:
        template<typename u_in>
        constexpr seqsubs(u_in& in, placeholder<intpc::val_v<U"0">>);
        template<typename u_in>
        constexpr seqsubs(u_in& in, intpc pos, placeholder<intpc::val_v<U"0">>);
        template<typename u_in>
        constexpr seqsubs(u_in& in, intpc pos, intpc len, placeholder<intpc::val_v<U"0">>)
        requires is_seqm_cpt<rm_qcv<u_in>>;
        template<typename u_in>
        constexpr seqsubs(u_in& in, intpc pos, intpc len, placeholder<intpc::val_v<U"0">>)
        requires is_seqsub_cpt<rm_qcv<u_in>>;
    private:
        using type_actual = t_type;
    public:
        using type = typename type_actual::type;
    private:
        using type_qualified = apply_qcv<type, type_actual>;

        friend struct seqsubs_access;
    public:
        constexpr seqsubs() = delete;

        template<typename t_self, is_seq_cpt u_in>
        constexpr auto operator=(this t_self&& self, const u_in& in) -> t_self&& requires (!is_qc_cpt<type_actual>);
        constexpr auto operator=(const seqsubs& in) -> seqsubs& = delete;
        
        constexpr explicit operator ptrm<seqsubs_access::type_qualified<t_type>>() const;

        constexpr auto len() const -> intpc;

        template<typename t_self>
        constexpr auto operator[](this t_self&& self, intpc pos) -> decltype(auto);
        constexpr auto operator[](intpc pos) const -> const type&;
    private:
        ptrs<type_actual> m_data{};
        intpc m_pos{};
        intpc m_len{};

        template<typename u_type>
        friend class seqmd;
        template<typename t_type>
        friend class seqsubs;
    };
}

#endif