#ifndef LGO_BASIC_TYPE_FUN_CAST_DECL
#define LGO_BASIC_TYPE_FUN_CAST_DECL

#include "../deduce/qualifier_rm.hpp"
#include "../deduce/qualifier_apply_basic.hpp"

namespace lgo {
	template<typename t_out, typename t_in>
	constexpr auto sc(t_in&& in) -> t_out;
	template<typename t_in>
	constexpr auto r(t_in&& in) -> rm_qr<t_in>&&;
	template<typename t_par, typename t_in>
	constexpr auto f(t_in&& in) -> apply_qr_basic<rm_qr<t_in>, t_par&&>;
}

#endif