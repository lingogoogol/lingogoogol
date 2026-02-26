#ifndef LGO_BASIC_TYPE_PTR_M_DEF
#define LGO_BASIC_TYPE_PTR_M_DEF

#include "decl.hpp"

#include "../main/def.hpp"
#include "../../tem/t/.hpp"
#include "../../lgo/.hpp"
#include "../../../type/deduce/qualifier_apply.hpp"

LGO_EXPORT namespace lgo {
	template<typename t_type>
	class ptrm: public lgo, public ptr<t_type> {
	private:
		using base = ptr<t_type>;
	public:
		template<typename... t_in>
		ptrm(t_in&&... in)
		requires requires { new(std::nothrow) rm_qr<typename base::type>{ ::lgo::f<t_in>(in)... }; };
		explicit ptrm(typename base::ptr_basic in);
		ptrm(const ptrm&) = delete;
		ptrm(ptrm&& in) noexcept;
		~ptrm();
		
		auto operator=(const ptrm&) -> ptrm& = delete;
		auto operator=(ptrm&& in) & -> ptrm&;
		
		auto s() const -> ptrs<t_type>;
		template<typename t_out>
		constexpr operator ptrm<t_out>() &&;
		template<typename t_out>
		constexpr operator ptrs<t_out&>() const;
		
		template<typename t_self>
		constexpr auto operator*(this t_self&& self) -> apply_qr<typename base::type, t_self>&&;
		template<typename t_self>
		constexpr auto ref(this t_self&& self) -> apply_qr<typename base::type, t_self>&&;
	};
}

#endif