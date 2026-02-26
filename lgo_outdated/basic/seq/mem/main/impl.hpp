#ifndef LGO_BASIC_SEQ_MEM_MAIN_IMPL
#define LGO_BASIC_SEQ_MEM_MAIN_IMPL

#include "def.hpp"

namespace lgo {
	template<is_mem_cpt t_self>
	constexpr auto mem::size(this const t_self& self, const size_mem<seq_plain>&) -> intpc {
		return self.len();
	}

	template<is_mems_cpt t_out, is_mem_cpt t_in>
	constexpr auto mem::mem_get(const t_in& in) -> t_out {
        if ((in.len() < t_out::len()).basic()) {
            throw get_not_enough{ t_out::len() - in.len() };
        }

		return t_out{ in.subcs(U"0"_intpc, t_out::len()) };
	}

	template<is_mem_cpt t_in, is_mem_cpt t_out>
	constexpr auto mem::mem_pad(const t_in& in, t_out& out) -> void {
		if constexpr (is_memd_cpt<t_in>) {
			throw e_other{ U"not implemented"_strmdc };
		}
		else if constexpr (is_mems_cpt<t_in>) {
			mem_pad(in, out, seq_plain{});
		}
		else {
			throw e_other{ U""_strmdc };
		}
		return;
	}

	template<is_mem_cpt t_in, is_mem_cpt t_out>
	constexpr auto mem::mem_pad(const t_in& in, t_out& out, seq_plain) -> void {
        if ((out.len() < in.size(size_mem{ seq_plain{} })).basic()) {
            throw pad_not_enough{ in.size(size_mem{ seq_plain{} }) - out.len() };
        }

		out.subms(U"0"_intpc, in.len()) = in;
		out.padpos(out.padpos() + in.len());
		return;
	}

	template<is_mem_cpt t_in, is_str_cpt t_out>
	constexpr auto mem::str_pad(const t_in& in, t_out& out) -> void {
        if ((out.len() < in.len() * U"2"_intpc).basic()) {
            throw pad_not_enough{ in.len() * U"2"_intpc - out.len() };
        }

		for (intpc i{ U"0"_intpc }; (i < in.len()).basic(); ++i) {
			out.pad(in[i]);
		}
		return;
	}
}

#endif