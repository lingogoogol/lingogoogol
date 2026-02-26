#ifndef LGO_BASIC_SEQ_MAIN_MS_NONEMPTY_DEF
#define LGO_BASIC_SEQ_MAIN_MS_NONEMPTY_DEF

#include "../s/impl.hpp"
#include "../cpt/.hpp"

namespace lgo {
    template<typename t_type, is_intp_v_cpt t_len>
    class seqms: public seqs {
    public:
        using type = t_type;
        using type_t = t<type>;

        constexpr static auto size(size_stack) -> intpc;
        constexpr static auto size(size_content) -> intpc
        requires requires { type::size(size_content{}); };
        constexpr auto size(size_content) -> intpc
        requires (!requires { type::size(size_content{}); });
        constexpr static auto len() -> intpc;

        constexpr seqms() = default;
        template<typename... t_in>
        constexpr seqms(t_in... in);

        template<is_seqs_cpt t_self, is_seq_cpt t_in>
        constexpr auto operator=(this t_self& self, const t_in& in) -> t_self&;

        template<typename t_self>
        constexpr auto operator[](this t_self& self, intpc pos) -> decltype(auto);
        template<typename t_self, is_intp_v_cpt t_pos>
        constexpr auto ind(this t_self& self) -> decltype(auto);
		template<typename t_self>
		constexpr auto data(this t_self&& self) -> auto;
    private:
        type m_data[t_len::val.basic()]{};
    };
}

#endif