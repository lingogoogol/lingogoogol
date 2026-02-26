#ifndef LGO_MATH_NUM_Z_F_BASE_DEF
#define LGO_MATH_NUM_Z_F_BASE_DEF

#include "base_decl.hpp"

#include "../main/def.hpp"

LGO_EXPORT namespace lgo {
    template<is_intp_cpt t_type, is_intp_v_cpt t_len>
    class numzf: public numz {
    public:
		using type = t_type;
		
		constexpr static auto len() -> intpc;
		constexpr static auto bits() -> intpc;
		constexpr auto lit(intsc in) -> void;
		constexpr auto lit() const -> intsc;
		constexpr auto big() const -> intsc;

		constexpr numzf() = default;
		template<is_int_cpt t_in>
		explicit constexpr numzf(t_in in, intsc lit = U"0"_intpc);
		template<is_numz_cpt t_in>
		constexpr numzf(const t_in& in, intsc lit = U"0"_intpc);
		constexpr numzf(const numzf& in) = default;

		using numz::operator=;
		constexpr auto operator=(const numzf&) & -> numzf& = default;

		template<typename t_self>
		constexpr auto operator[](this t_self&& self, intsc pos) -> decltype(auto);
	private:
		arrms<type, t_len> m_data{};
        intsc m_lit{};
    };
}

#endif