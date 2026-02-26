#ifndef LGO_BASIC_TYPE_PTR_M_IMPL
#define LGO_BASIC_TYPE_PTR_M_IMPL

#include "def.hpp"

#include "../main/impl.hpp"

namespace lgo {
	template<typename t_type>
	template<typename... t_in>
	ptrm<t_type>::ptrm(t_in&&... in)
	requires requires { new(std::nothrow) rm_qr<typename base::type>{ ::lgo::f<t_in>(in)... }; }
	: base{ new(std::nothrow) rm_qr<typename base::type>{ ::lgo::f<t_in>(in)... } } {
		while (!this->m_data) {
			this->m_data = new(std::nothrow) rm_qr<typename base::type>{ ::lgo::f<t_in>(in)... };
		}
		return;
	}

	template<typename t_type>
	ptrm<t_type>::ptrm(typename base::ptr_basic in): base{ in } {}

	template<typename t_type>
	ptrm<t_type>::ptrm(ptrm&& in) noexcept: base{ in.m_data } {
		in.m_data = nullptr;
		return;
	}

	template<typename t_type>
	ptrm<t_type>::~ptrm() {
		delete this->m_data;
		return;
	}

	template<typename t_type>
	auto ptrm<t_type>::operator=(ptrm&& in) & -> ptrm& {
		if (this->m_data != in.m_data) {
			delete this->m_data;
			this->m_data = in.m_data;
			in.m_data = nullptr;
		}
		return *this;
	}
	
	template<typename t_type>
	auto ptrm<t_type>::s() const -> ptrs<t_type> {
		return ptrs<t_type>{ this->m_data };
	}

	template<typename t_type>
	template<typename t_out>
	constexpr ptrm<t_type>::operator ptrm<t_out>() && {
		ptrm<t_out> out{ ::lgo::sc<t_out*>(this->m_data) };
		this->m_data = nullptr;
		return out;
	}

	template<typename t_type>
	template<typename t_out>
	constexpr ptrm<t_type>::operator ptrs<t_out&>() const {
		return ptrs<t_out&>{ ::lgo::sc<t_out*>(base::m_data) };
	}

	template<typename t_type>
	template<typename t_self>
	constexpr auto ptrm<t_type>::operator*(this t_self&& self) -> apply_qr<typename base::type, t_self>&& {
		return ::lgo::sc<apply_qr<typename base::type, t_self>&&>(*self.m_data);
	}

	template<typename t_type>
	template<typename t_self>
	constexpr auto ptrm<t_type>::ref(this t_self&& self) -> apply_qr<typename base::type, t_self>&& {
		return *self;
	}
}

#endif