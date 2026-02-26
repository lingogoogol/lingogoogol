#ifndef LGO_BASIC_SEQ_MEM_MS_IMPL
#define LGO_BASIC_SEQ_MEM_MS_IMPL

#include "def.hpp"

#include "../subs/.hpp"
#include "../main/impl.hpp"
#include "../../io/s/impl.hpp"

namespace lgo {
	template<is_intp_v_cpt t_len>
	template<is_mem_cpt t_in>
	constexpr memms<t_len>::memms(const t_in& in) {
		if ((in.len() < base::len()).basic()) {
			throw e_other{ U""_strmdc };
		}
		for (intpc i{ U"0"_intpc }; (i < base::len()).basic(); ++i) {
			(*this)[i] = in[i];
		}
		in.getpos(base::len());
		return;
	}
	
	template<is_intp_v_cpt t_len>
	constexpr auto memms<t_len>::operator=(const memms& in) & -> memms& {
		return base::template operator=(in);
	}

	template<is_intp_v_cpt t_len>
	template<is_memms_cpt t_in>
	constexpr auto memms<t_len>::operator+(const t_in& in) const -> memms<v<base::len() + t_in::len()>> {
		return memms<base::len() + t_in::len()>{ this->m_data + in.m_data };
	}

	template<is_intp_v_cpt t_len>
	template<typename t_self, typename... t_opt>
	constexpr auto memms<t_len>::subms(this t_self& self, t_opt... opt) -> memsubs<t_self> {
		return memsubs<t_self>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
	}

	template<is_intp_v_cpt t_len>
	template<typename t_self, typename... t_opt>
	constexpr auto memms<t_len>::subcs(this t_self& self, t_opt... opt) -> memsubs<const t_self> {
		return memsubs<const t_self>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
	}
}

#endif