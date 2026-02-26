#ifndef LGO_BASIC_TYPE_DEDUCE_QUALIFIER_APPLY
#define LGO_BASIC_TYPE_DEDUCE_QUALIFIER_APPLY

#include "cond.hpp"

#include "../constraint/qualifier.hpp"
#include "../boo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_target, typename t_in>
    using apply_qc = tcond<arrmc<is_qc_v<t_in>, const t_target>, arrmc<boo::val_v<U"1">, t_target>>;
    template<typename t_target, typename t_in>
    using apply_qv = tcond<arrmc<is_qv_v<t_in>, volatile t_target>, arrmc<boo::val_v<U"1">, t_target>>;

	template<typename t_target, typename t_in>
	using apply_qp = tcond<arrmc<is_qp_v<t_in>, t_target*>, arrmc<boo::val_v<U"1">, t_target>>;
	template<typename t_target, typename t_in>
	using apply_qlr = tcond<arrmc<is_qlr_v<t_in>, t_target&>, arrmc<boo::val_v<U"1">, t_target>>;
	template<typename t_target, typename t_in>
	using apply_qrr = tcond<arrmc<is_qrr_v<t_in>, t_target&&>, arrmc<boo::val_v<U"1">, t_target>>;
	
	template<typename t_target, typename t_in>
	using apply_qcv = apply_qc<apply_qv<t_target, t_in>, t_in>;
	template<typename t_target, typename t_in>
	using apply_qr = tcond<arrmc<is_qlr_v<t_in>, t_target&>, arrmc<is_qrr_v<t_in>, t_target&&>, arrmc<boo::val_v<U"1">, t_target>>;

    template<typename t_target, typename t_in>
    using apply_qc_t = t<apply_qc<t_target, t_in>>;
    template<typename t_target, typename t_in>
    using apply_qv_t = t<apply_qv<t_target, t_in>>;

    template<typename t_target, typename t_in>
    using apply_qp_t = t<apply_qp<t_target, t_in>>;
    template<typename t_target, typename t_in>
    using apply_qlr_t = t<apply_qlr<t_target, t_in>>;
    template<typename t_target, typename t_in>
    using apply_qrr_t = t<apply_qrr<t_target, t_in>>;
    
    template<typename t_target, typename t_in>
    using apply_qcv_t = t<apply_qcv<t_target, t_in>>;
    template<typename t_target, typename t_in>
    using apply_qr_t = t<apply_qr<t_target, t_in>>;
}

#endif