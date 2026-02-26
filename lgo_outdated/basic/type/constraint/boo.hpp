#ifndef LGO_BASIC_TYPE_CONSTRAINT_BOO
#define LGO_BASIC_TYPE_CONSTRAINT_BOO

#include "qualifier.hpp"
#include "common.hpp"
#include "cmp.hpp"

#include "../boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in>
    constexpr inline boo is_qc_boo{ is_qc_cpt<t_in> };
    template<typename t_in>
    using is_qc_v = v<is_qc_boo<t_in>>;
    template<typename t_in>
    using is_qc_v_t = t<is_qc_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_qv_boo{ is_qv_cpt<t_in> };
    template<typename t_in>
    using is_qv_v = v<is_qv_boo<t_in>>;
    template<typename t_in>
    using is_qv_v_t = t<is_qv_v<t_in>>;

    template<typename t_in>
    constexpr inline boo is_qp_boo{ is_qp_cpt<t_in> };
    template<typename t_in>
    using is_qp_v = v<is_qp_boo<t_in>>;
    template<typename t_in>
    using is_qp_v_t = t<is_qp_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_qlr_boo{ is_qlr_cpt<t_in> };
    template<typename t_in>
    using is_qlr_v = v<is_qlr_boo<t_in>>;
    template<typename t_in>
    using is_qlr_v_t = t<is_qlr_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_qrr_boo{ is_qrr_cpt<t_in> };
    template<typename t_in>
    using is_qrr_v = v<is_qrr_boo<t_in>>;
    template<typename t_in>
    using is_qrr_v_t = t<is_qrr_v<t_in>>;

    template<typename t_in>
    constexpr inline boo is_qr_boo{ is_qr_cpt<t_in> };
    template<typename t_in>
    using is_qr_v = v<is_qr_boo<t_in>>;
    template<typename t_in>
    using is_qr_v_t = t<is_qr_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_qcv_boo{ is_qcv_cpt<t_in> };
    template<typename t_in>
    using is_qcv_v = v<is_qcv_boo<t_in>>;
    template<typename t_in>
    using is_qcv_v_t = t<is_qcv_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_qcr_boo{ is_qcr_cpt<t_in> };
    template<typename t_in>
    using is_qcr_v = v<is_qcr_boo<t_in>>;
    template<typename t_in>
    using is_qcr_v_t = t<is_qcr_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_qvr_boo{ is_qvr_cpt<t_in> };
    template<typename t_in>
    using is_qvr_v = v<is_qvr_boo<t_in>>;
    template<typename t_in>
    using is_qvr_v_t = t<is_qvr_v<t_in>>;
    template<typename t_in>
    constexpr inline boo is_qcvr_boo{ is_qcvr_cpt<t_in> };
    template<typename t_in>
    using is_qcvr_v = v<is_qcvr_boo<t_in>>;
    template<typename t_in>
    using is_qcvr_v_t = t<is_qcvr_v<t_in>>;

	template<typename... t_in>
	constexpr inline boo is_same_boo{ is_same_cpt<t_in...> };
    template<typename... t_in>
    using is_same_v = v<is_same_boo<t_in...>>;
    template<typename... t_in>
    using is_same_v_t = t<is_same_v<t_in...>>;
	template<typename... t_in>
	constexpr inline boo is_one_of_boo{ is_one_of_cpt<t_in...> };
    template<typename... t_in>
    using is_one_of_v = v<is_one_of_boo<t_in...>>;
    template<typename... t_in>
    using is_one_of_v_t = t<is_one_of_v<t_in...>>;

	template<typename... t_in>
	constexpr inline boo is_constructible_boo{ is_constructible_cpt<t_in...> };
    template<typename... t_in>
    using is_constructible_v = v<is_constructible_boo<t_in...>>;
    template<typename... t_in>
    using is_constructible_v_t = t<is_constructible_v<t_in...>>;
	template<typename t_in1, typename t_in2>
	constexpr inline boo is_assignable_boo{ is_assignable_cpt<t_in1, t_in2> };
    template<typename t_in1, typename t_in2>
    using is_assignable_v = v<is_assignable_boo<t_in1, t_in2>>;
    template<typename t_in1, typename t_in2>
    using is_assignable_v_t = t<is_assignable_v<t_in1, t_in2>>;
	template<typename t_in1, typename t_in2>
	constexpr inline boo is_comparable_boo{ is_comparable_cpt<t_in1, t_in2> };
    template<typename t_in1, typename t_in2>
    using is_comparable_v = v<is_comparable_boo<t_in1, t_in2>>;
    template<typename t_in1, typename t_in2>
    using is_comparable_v_t = t<is_comparable_v<t_in1, t_in2>>;
}

#endif