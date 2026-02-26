#ifndef LGO_BASIC_SEQ_MEM_SUBS_IMPL
#define LGO_BASIC_SEQ_MEM_SUBS_IMPL

#include "def.hpp"

namespace lgo {
    template<typename t_type>
    constexpr auto memsubs<t_type>::operator=(const memsubs& in) & -> memsubs& {
        return base::template operator=(in);
    }

    template<typename t_type>
    template<typename t_self, typename... t_opt>
    constexpr auto memsubs<t_type>::subcs(this t_self& self, t_opt... opt) -> memsubs<apply_qcv<const t_type, t_self>> {
        return memsubs<apply_qcv<const t_type, t_self>>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }

    template<typename t_type>
    template<typename t_self, typename... t_opt>
    constexpr auto memsubs<t_type>::subms(this t_self& self, t_opt... opt) -> memsubs<apply_qcv<t_type, t_self>> {
        return memsubs<apply_qcv<t_type, t_self>>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }
}

#endif