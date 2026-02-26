#ifndef LGO_BASIC_TYPE_DEDUCE_QUALIFIER_RM
#define LGO_BASIC_TYPE_DEDUCE_QUALIFIER_RM

#include "qualifier_stu.hpp"

#include "../tem/t/decl.hpp"

LGO_EXPORT namespace lgo {
	template<typename t_in>
	using rm_qc = qc_stu<t_in>::type;
	template<typename t_in>
	using rm_qv = qv_stu<t_in>::type;

	template<typename t_in>
	using rm_qp = qp_stu<t_in>::type;
	template<typename t_in>
	using rm_qlr = qlr_stu<t_in>::type;
	template<typename t_in>
	using rm_qrr = qrr_stu<t_in>::type;
	
	template<typename t_in>
	using rm_qr = rm_qlr<rm_qrr<t_in>>;
	template<typename t_in>
	using rm_qcv = rm_qc<rm_qv<t_in>>;
	template<typename t_in>
	using rm_qcr = rm_qc<rm_qr<t_in>>;
	template<typename t_in>
	using rm_qvr = rm_qv<rm_qr<t_in>>;
	template<typename t_in>
	using rm_qcvr = rm_qcv<rm_qr<t_in>>;
	
	template<typename t_in>
	using rm_qc_t = t<rm_qc<t_in>>;
	template<typename t_in>
	using rm_qv_t = t<rm_qv<t_in>>;

	template<typename t_in>
	using rm_qp_t = t<rm_qp<t_in>>;
	template<typename t_in>
	using rm_qlr_t = t<rm_qlr<t_in>>;
	template<typename t_in>
	using rm_qrr_t = t<rm_qrr<t_in>>;
	
	template<typename t_in>
	using rm_qr_t = t<rm_qr<t_in>>;
	template<typename t_in>
	using rm_qcv_t = t<rm_qcv<t_in>>;
	template<typename t_in>
	using rm_qcr_t = t<rm_qcr<t_in>>;
	template<typename t_in>
	using rm_qvr_t = t<rm_qvr<t_in>>;
	template<typename t_in>
	using rm_qcvr_t = t<rm_qcvr<t_in>>;
}

#endif