#ifndef LGO_BASIC_SEQ_MEM_MAIN_DEF
#define LGO_BASIC_SEQ_MEM_MAIN_DEF

#include "decl.hpp"

namespace lgo {
	class mem {
	protected:
		constexpr mem() = default;
	public:
		template<is_mem_cpt t_self>
        constexpr auto size(this const t_self& self, const size_mem<seq_plain>&) -> intpc;

		template<is_mems_cpt t_out, is_mem_cpt t_in>
		constexpr static auto mem_get(const t_in& in) -> t_out;
		template<is_mem_cpt t_in, is_mem_cpt t_out>
		constexpr static auto mem_pad(const t_in& in, t_out& out) -> void;
		template<is_mem_cpt t_in, is_mem_cpt t_out>
		constexpr static auto mem_pad(const t_in& in, t_out& out, seq_plain) -> void;
		template<is_mem_cpt t_in, is_str_cpt t_out>
		constexpr static auto str_pad(const t_in& in, t_out& out) -> void;
	};
}

#endif