#ifndef LGO_BASIC_TYPE_PTR_MAIN_DEF
#define LGO_BASIC_TYPE_PTR_MAIN_DEF

#include "cpt.hpp"

#include "../s/decl.hpp"
#include "../../tem/t/.hpp"
#include "../../int/p/alias.hpp"
#include "../../boo/decl.hpp"

namespace lgo {
	template<typename t_type>
	class ptr {
	protected:
		using ptr_basic = rm_qr<t_type>*;
    public:
		using type = t_type;
		using type_t = t<type>;

		template<typename t_in>
		constexpr static auto size(t_in) -> intpc
        requires is_one_of_cpt<t_in, size_stack, size_content>;
		
		constexpr ptr() = default;
		constexpr explicit ptr(ptr_basic in);

		constexpr auto operator=(const ptr&) & -> ptr& = default;
		
		constexpr auto basic() const -> ptr_basic;
		template<typename t_out>
		auto dc() const -> ptrs<t_out&>;

		template<is_ptr_cpt t_self, is_ptr_cpt t_in>
		constexpr auto operator==(this t_self self, t_in in) -> boo;
	protected:
		ptr_basic m_data{ nullptr };

		template<typename t_type>
		friend class ptr;
    };
}

#endif