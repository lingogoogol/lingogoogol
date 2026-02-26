#ifndef LGO_MATH_NUM_Z_S_BASE_IMPL
#define LGO_MATH_NUM_Z_S_BASE_IMPL

#include "base_def.hpp"

#include "../main/impl.hpp"

namespace lgo {
	template<is_intp_cpt t_type, is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	template<is_size_mem_cpt t_in>
	constexpr auto numzs<t_type, t_lit, t_len>::size(const t_in&) -> intpc {
		return arrms<type, t_len>::size(size_content{});
	}

	template<is_intp_cpt t_type, is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	constexpr auto numzs<t_type, t_lit, t_len>::len() -> intpc {
		return t_len::val;
	}

	template<is_intp_cpt t_type, is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	constexpr auto numzs<t_type, t_lit, t_len>::bits() -> intpc {
		return type::bits() * len();
	}

	template<is_intp_cpt t_type, is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	constexpr auto numzs<t_type, t_lit, t_len>::lit() -> intsc {
		return t_lit::val;
	}

	template<is_intp_cpt t_type, is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	constexpr auto numzs<t_type, t_lit, t_len>::big() -> intsc {
		return lit() + len() - U"1"_intpc;
	}
	
	template<is_intp_cpt t_type, is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	template<is_numz_impl_cpt t_in, is_mem_cpt t_out, typename... t_opt>
	constexpr auto numzs<t_type, t_lit, t_len>::mem_pad(const t_in& in, t_out& out, t_opt...) -> void {
		using opt_arr_type = arrmc<t_opt...>;
		if constexpr ((!opt_arr_type::template tcontain<bind<is_same_bind, bind_arg<U"0">, numz_no_sign>>).basic()) {
			throw e_other{ U"not implemented"_strmdc };
		}
		else {
			if constexpr (opt_arr_type::template tcontain<bind<is_same_bind, bind_arg<U"0">, numz_big_endian>>.basic()) {
				out.pad(in.m_data, seq_plain{}, seq_inverse{});
			}
			else {
				out.pad(in.m_data, seq_plain{});
			}
			return;
		}
	}
	
	template<is_intp_cpt t_type, is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	template<is_int_cpt t_in>
	constexpr numzs<t_type, t_lit, t_len>::numzs(t_in in): numz{ in } {
		ctor(in);
		return;
	}

	template<is_intp_cpt t_type, is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	template<is_numz_cpt t_in>
	constexpr numzs<t_type, t_lit, t_len>::numzs(const t_in& in) {
		ctor(in);
		return;
	}

	template<is_intp_cpt t_type, is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	template<typename t_self>
	constexpr auto numzs<t_type, t_lit, t_len>::operator[](this t_self&& self, intsc pos) -> decltype(auto) {
		return self.m_data[self.pos_to_index(pos)];
	}
}

#endif