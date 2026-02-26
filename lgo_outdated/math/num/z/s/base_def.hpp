#ifndef LGO_MATH_NUM_Z_S_BASE_DEF
#define LGO_MATH_NUM_Z_S_BASE_DEF

#include "../main/def.hpp"
#include "../main/opt/.hpp"

namespace lgo {
	template<is_intp_cpt t_type, is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	class numzs: public numz {
	public:
		using type = t_type;
		
		template<is_size_mem_cpt t_in>
		constexpr static auto size(const t_in&) -> intpc;
		constexpr static auto len() -> intpc;
		constexpr static auto bits() -> intpc;
		constexpr static auto lit() -> intsc;
		constexpr static auto big() -> intsc;

		template<is_numz_impl_cpt t_in, is_mem_cpt t_out, typename... t_opt>
		constexpr static auto mem_pad(const t_in& in, t_out& out, t_opt...) -> void;

		constexpr numzs() = default;
		template<is_int_cpt t_in>
		explicit constexpr numzs(t_in in);
		template<is_numz_cpt t_in>
		constexpr numzs(const t_in& in);
		constexpr numzs(const numzs& in) = default;

		using numz::operator=;
		constexpr auto operator=(const numzs&) & -> numzs& = default;

		template<typename t_self>
		constexpr auto operator[](this t_self&& self, intsc pos) -> decltype(auto);
	private:
		arrms<type, t_len> m_data{};
	};
}

#endif