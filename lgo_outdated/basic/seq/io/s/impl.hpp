#ifndef LGO_BASIC_SEQ_IO_S_IMPL
#define LGO_BASIC_SEQ_IO_S_IMPL

#include "def.hpp"

#include "../../../io/e/.hpp"

namespace lgo {
	template<typename t_out, is_str_cpt t_in, typename... t_param>
	constexpr auto seqios::get_call(const t_in& in, t_param&&... param) const -> t_out {
		return t_out::template str_get<t_out>(in, ::lgo::f<t_param>(param)...);
	}

	template<typename t_out, is_mem_cpt t_in, typename... t_param>
	constexpr auto seqios::get_call(const t_in& in, t_param&&... param) const -> t_out {
		return t_out::template mem_get<t_out>(in, ::lgo::f<t_param>(param)...);
	}

	template<typename t_in, is_str_cpt t_out, typename... t_param>
	constexpr auto seqios::pad_call(const t_in& in, t_out& out, t_param&&... param) const -> void {
		t_in::str_pad(in, out, ::lgo::f<t_param>(param)...);
		return;
	}

	template<typename t_in, is_mem_cpt t_out, typename... t_param>
	constexpr auto seqios::pad_call(const t_in& in, t_out& out, t_param&&... param) const -> void {
		t_in::mem_pad(in, out, ::lgo::f<t_param>(param)...);
		return;
	}

	template<typename t_out, is_seq_cpt t_self, typename... t_param>
	constexpr auto seqios::get(this const t_self& self, t_param&&... param) -> t_out {
		auto sub{ self.subms(self.getpos()) };
		t_out out{ self.get_call<t_out>(sub, ::lgo::f<t_param>(param)...) };
		self.getpos(self.getpos() + sub.getpos());
		return out;
	}

	template<typename t_out, is_seq_cpt t_self, typename... t_param>
	constexpr auto seqios::get_silent(this const t_self& self, t_param&&... param) -> t_out {
		auto sub{ self.subms(self.getpos()) };
		t_out out{ self.get_call<t_out>(sub, ::lgo::f<t_param>(param)...) };
		return out;
	}

	template<typename t_out, is_seq_cpt t_self, typename... t_param>
	constexpr auto seqios::get_try(this const t_self& self, t_param&&... param) -> t_out {
		return self.get_call<t_out>(self, ::lgo::f<t_param>(param)...);
	}

	template<typename t_self, typename t_in, typename... t_param>
	constexpr auto seqios::pad(this t_self&& self, const t_in& in, t_param&&... param) -> t_self&& {
		auto sub{ self.subms(self.padpos()) };
		self.pad_call(in, sub, ::lgo::f<t_param>(param)...);
		self.padpos(self.padpos() + sub.padpos());
		return self.f<t_self>();
	}

	constexpr auto seqios::getpos() const -> intpc {
		return m_getpos;
	}

	template<typename t_self>
	constexpr auto seqios::getpos(this t_self&& self, intpc in) -> t_self&& {
		self.m_getpos = in;
		return self.f<t_self>();
	}

	constexpr auto seqios::padpos() const -> intpc {
		return m_padpos;
	}

	template<typename t_self>
	constexpr auto seqios::padpos(this t_self&& self, intpc in) -> t_self&& {
		self.m_padpos = in;
		return self.f<t_self>();
	}
}

#endif