#ifndef LGO_BASIC_TYPE_INT_P_DEF
#define LGO_BASIC_TYPE_INT_P_DEF

#include "decl.hpp"
#include "alias.hpp"

#include "../s/basic.hpp"
#include "../u/def.hpp"
#include "../main/cpt.hpp"
#include "../../lgo/def.hpp"
#include "../../tem/t/def.hpp"

LGO_EXPORT namespace lgo {
	template<typename t_type>
	class intp_class: public lgo, public intu_class<t_type, intp_class<t_type>> {
	public:
		static_assert(is_basic_intp_cpt<t_type>);
	private:
		using base = intu_class<t_type, intp_class<t_type>>;
	public:
		using base::size;
        template<typename t_in>
		constexpr static auto size(t_in in) -> intpc
        requires is_one_of_cpt<t_in, size_stack, size_content>;
        constexpr static auto min() -> intp_class;
        constexpr static auto max() -> intp_class;
		
		constexpr intp_class() = default;
		template<is_basic_int_cpt t_in>
		constexpr explicit intp_class(t_in in);
		template<is_int_cpt t_in>
		constexpr intp_class(const t_in& in);
		constexpr intp_class(const intp_class& in);

		template<is_int_cpt t_in>
		constexpr auto operator=(t_in in) & -> intp_class&;
		constexpr auto operator=(const intp_class& in) & -> intp_class&;

		constexpr auto basic() const -> typename base::type;

		using base::operator+;
		using base::operator-;
        constexpr auto sign() const -> boo;
        constexpr auto absp() const -> intp_class;
        constexpr auto operator+() -> intp_class;
        constexpr auto operator-() const -> intn_class<typename base::type>;

		template<typename t_self>
		constexpr auto operator++(this t_self&& self) -> t_self&&;
		constexpr auto operator++(int) -> intp_class;
		template<typename t_self>
		constexpr auto operator--(this t_self&& self) -> t_self&&;
		constexpr auto operator--(int) -> intp_class;
	};
}

#endif