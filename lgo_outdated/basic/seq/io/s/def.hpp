#ifndef LGO_BASIC_SEQ_IO_S_DEF
#define LGO_BASIC_SEQ_IO_S_DEF

namespace lgo {
    class seqios {
	private:
		template<typename t_out, is_str_cpt t_in, typename... t_param>
		constexpr auto get_call(const t_in& in, t_param&&... param) const -> t_out;
		template<typename t_out, is_mem_cpt t_in, typename... t_param>
		constexpr auto get_call(const t_in& in, t_param&&... param) const -> t_out;
		template<typename t_in, is_str_cpt t_out, typename... t_param>
		constexpr auto pad_call(const t_in& in, t_out& out, t_param&&... param) const -> void;
		template<typename t_in, is_mem_cpt t_out, typename... t_param>
		constexpr auto pad_call(const t_in& in, t_out& out, t_param&&... param) const -> void;
    public:
		template<typename t_out, is_seq_cpt t_self, typename... t_param>
		constexpr auto get(this const t_self& self, t_param&&... param) -> t_out;
		template<typename t_out, is_seq_cpt t_self, typename... t_param>
		constexpr auto get_silent(this const t_self& self, t_param&&... param) -> t_out;
		template<typename t_out, is_seq_cpt t_self, typename... t_param>
		constexpr auto get_try(this const t_self& self, t_param&&... param) -> t_out;
		template<typename t_self, typename t_in, typename... t_param>
		constexpr auto pad(this t_self&& self, const t_in& in, t_param&&... param) -> t_self&&;

		constexpr auto getpos() const -> intpc;
		template<typename t_self>
		constexpr auto getpos(this t_self&& self, intpc in) -> t_self&&;
		constexpr auto padpos() const -> intpc;
		template<typename t_self>
		constexpr auto padpos(this t_self&& self, intpc in) -> t_self&&;
	private:
		mutable intpc m_getpos{ U"0"_intpc };
		mutable intpc m_padpos{ U"0"_intpc };
    };
}

#endif