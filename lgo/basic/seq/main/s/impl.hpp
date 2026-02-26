#ifndef LGO_BASIC_SEQ_MAIN_S_IMPL
#define LGO_BASIC_SEQ_MAIN_S_IMPL

#include "def.hpp"

#include "../c/impl.hpp"
#include "../../../type/int/p/literal.hpp"

namespace lgo {
    template<typename t_self, typename t_in>
    constexpr auto seqs::operator==(this const t_self& self, const t_in& in) -> boo {
		if ((self.len() != in.len()).basic()) {
			return U"0"_boo;
		}
		for (intpc i{ U"0"_intpc }; (i < self.len()).basic(); ++i) {
			if ((self[i] != in[i]).basic()) {
				return U"0"_boo;
			}
		}
		return U"1"_boo;
    }

    template<typename t_self, is_fun_cpt t_fun>
    constexpr auto seqs::each(this t_self&& self, const t_fun& fun) -> void {
        for (intpc i{ U"0"_intpc }; i < self.len(); ++i) {
            fun(self[i]);
        }
        return;
    }

    template<typename t_self, is_fun_cpt t_fun>
    constexpr auto seqs::find(this t_self&& self, const t_fun& fun) -> intpc {
        for (intpc i{ U"0"_intpc }; i < self.len(); ++i) {
            if (fun(self[i]).basic()) {
                return i;
            }
        }
        throw e_other{ U""_strmdc };
    }
}

#endif