#ifndef LGO_BASIC_TYPE_FUN_CAST_IMPL
#define LGO_BASIC_TYPE_FUN_CAST_IMPL

#include "cast_decl.hpp"

LGO_EXPORT namespace lgo {
	template<typename t_out, typename t_in>
	constexpr auto sc(t_in&& in) -> t_out {
		return static_cast<t_out>(static_cast<t_in&&>(in));
	}

	template<typename t_in, typename t_out>
	concept is_scable_cpt = requires(t_in&& in) {
		{ sc<t_out>(in) };
	};

	template<typename t_in>
	constexpr auto r(t_in&& in) -> rm_qr<t_in>&& {
        return sc<rm_qr<t_in>&&>(in);
	}

	template<typename t_par, typename t_in>
	constexpr auto f(t_in&& in) -> apply_qr_basic<rm_qr<t_in>, t_par&&> {
        return sc<apply_qr_basic<rm_qr<t_in>, t_par&&>>(in);
	}
}

#endif