#ifndef LGO_MATH_NUM_Z_S_CVT
#define LGO_MATH_NUM_Z_S_CVT

#include "base_decl.hpp"

namespace lgo {
	template<is_numzs_impl_cpt t_in>
	struct numzs_to_interface_stu {};

	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	struct numzs_to_interface_stu<numzs<intp8, t_lit, t_len>> {
        using type = numzs8<t_lit, t_len>;
    };

	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	struct numzs_to_interface_stu<numzs<intp16, t_lit, t_len>> {
        using type = numzs16<t_lit, t_len>;
    };

	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	struct numzs_to_interface_stu<numzs<intp32, t_lit, t_len>> {
        using type = numzs32<t_lit, t_len>;
    };

	template<is_int_v_cpt t_lit, is_intp_v_cpt t_len>
	struct numzs_to_interface_stu<numzs<intp64, t_lit, t_len>> {
        using type = numzs64<t_lit, t_len>;
    };

	template<is_numzs_impl_cpt t_in>
	using numzs_to_interface = numzs_to_interface_stu<t_in>::type;
}

#endif