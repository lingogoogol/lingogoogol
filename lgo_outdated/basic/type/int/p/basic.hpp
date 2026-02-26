#ifndef LGO_BASIC_TYPE_INT_P_BASIC
#define LGO_BASIC_TYPE_INT_P_BASIC

#include "../../constraint/common.hpp"
#include "../../tem/t/decl.hpp"
#include "../../tem/t/cpt.hpp"

namespace lgo {
	using basic_intp8 = std::uint8_t;
	using basic_intp8_t = t<basic_intp8>;
	using basic_intp16 = std::uint16_t;
	using basic_intp16_t = t<basic_intp16>;
	using basic_intp32 = std::uint32_t;
	using basic_intp32_t = t<basic_intp32>;
	using basic_intp64 = std::uint64_t;
	using basic_intp64_t = t<basic_intp64>;
	
	using basic_intpc = basic_intp64;
	using basic_intpc_t = t<basic_intpc>;

	template<typename t_in>
	concept is_basic_intp_cpt = is_one_of_cpt<t_in, unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long>;
	template<typename t_in>
	concept is_basic_intp_t_cpt = is_t_cpt<t_in> && is_basic_intp_cpt<typename t_in::type>;
}

#endif