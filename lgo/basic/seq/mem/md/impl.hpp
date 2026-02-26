#ifndef LGO_BASIC_SEQ_MEM_MD_IMPL
#define LGO_BASIC_SEQ_MEM_MD_IMPL

#include "def.hpp"

#include "../main/impl.hpp"
#include "../../main/subs/.hpp"
#include "../../io/d/impl.hpp"

namespace lgo {
	memmd::memmd(const memmd& in): base{ in } {
		base::ctor(in);
		return;
	}

	template<is_seq_impl_cpt t_in>
	memmd::memmd(const t_in& in)
	requires is_same_cpt<typename t_in::type, byte>: base{ in } {
		base::ctor(in);
		return;
	}

	auto memmd::operator=(const memmd& in) & -> memmd& {
		return base::template operator=(in);
	}

	auto memmd::operator=(memmd&& in) & -> memmd& {
		return base::template operator=(in.r());
	}

    template<typename t_self, typename... t_opt>
    auto memmd::subcs(this t_self& self, t_opt... opt) -> memsubs<const t_self> {
        return memsubs<const t_self>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }

    template<typename t_self, typename... t_opt>
    auto memmd::subms(this t_self& self, t_opt... opt) -> memsubs<t_self> {
        return memsubs<t_self>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }

    template<typename t_self, typename... t_opt>
    auto memmd::subcd(this t_self& self, t_opt... opt) -> memsubd<const t_self> {
        return memsubd<const t_self>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }

    template<typename t_self, typename... t_opt>
    auto memmd::submd(this t_self& self, t_opt... opt) -> memsubd<t_self> {
        return memsubd<t_self>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }
}

#endif