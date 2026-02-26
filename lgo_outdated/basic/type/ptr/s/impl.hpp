#ifndef LGO_BASIC_TYPE_PTR_S_IMPL
#define LGO_BASIC_TYPE_PTR_S_IMPL

#include "def.hpp"

#include "../main/impl.hpp"

namespace lgo {
    template<typename t_type>
    constexpr ptrs<t_type>::ptrs(t_type&& in): base{ std::addressof(in) } {}

    template<typename t_type>
	constexpr ptrs<t_type>::ptrs(rm_qr<t_type>* in): base{ in } {}

	template<typename t_type>
	constexpr ptrs<t_type>::operator ptrs<const rm_qr<t_type>&>() const & {
		return ptrs<const rm_qr<t_type>&>{ base::m_data };
	}

	template<typename t_type>
	constexpr ptrs<t_type>::operator ptrs<const rm_qr<t_type>>() const && {
		return ptrs<const rm_qr<t_type>>{ base::m_data };
	}

	template<typename t_type>
	template<typename t_out>
	constexpr ptrs<t_type>::operator ptrs<t_out&>() const & {
		return ptrs<t_out&>{ ::lgo::sc<t_out*>(base::m_data) };
	}

	template<typename t_type>
	template<typename t_out>
	constexpr ptrs<t_type>::operator ptrs<t_out>() const && {
		return ptrs<t_out>{ ::lgo::sc<t_out*>(base::m_data) };
	}

	template<typename t_type>
	template<typename t_self>
	constexpr auto ptrs<t_type>::operator*(this t_self&& self) -> apply_qr<apply_qr<typename base::type, t_type>, t_self>&& {
		return ::lgo::sc<apply_qr<apply_qr<typename base::type, t_type>, t_self>&&>(*self.m_data);
	}

	template<typename t_type>
	template<typename t_self>
	constexpr auto ptrs<t_type>::ref(this t_self&& self) -> apply_qr<apply_qr<typename base::type, t_type>, t_self>&& {
		return *self.f<t_self>();
	}
}

#endif