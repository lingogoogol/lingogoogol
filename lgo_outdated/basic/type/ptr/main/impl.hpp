#ifndef LGO_BASIC_TYPE_PTR_MAIN_IMPL
#define LGO_BASIC_TYPE_PTR_MAIN_IMPL

#include "def.hpp"

#include "../../int/p/life.hpp"

namespace lgo {
	template<typename t_type>
	template<typename t_in>
	constexpr auto ptr<t_type>::size(t_in) -> intpc
	requires is_one_of_cpt<t_in, size_stack, size_content> {
		return intpc{ sizeof(type*) };
	}

	template<typename t_type>
	constexpr ptr<t_type>::ptr(ptr_basic in): m_data{ in } {}

	template<typename t_type>
	constexpr auto ptr<t_type>::basic() const -> ptr_basic {
		return m_data;
	}
	
	template<typename t_type>
	template<typename t_out>
	auto ptr<t_type>::dc() const -> ptrs<t_out&> {
		return ptrs<t_out&>{ dynamic_cast<t_out*>(this->m_data) };
	}

	template<typename t_type>
	template<is_ptr_cpt t_self, is_ptr_cpt t_in>
	constexpr auto ptr<t_type>::operator==(this t_self self, t_in in) -> boo {
		return boo{ self.m_data == in.m_data };
	}
}

#endif