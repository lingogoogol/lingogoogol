#ifndef LGO_BASIC_SEQ_STR_SUBS_IMPL
#define LGO_BASIC_SEQ_STR_SUBS_IMPL

#include "def.hpp"

#include "../main/def.hpp"
#include "../../main/subs/impl.hpp"

namespace lgo {
    template<typename t_type>
    constexpr auto strsubs<t_type>::operator=(const strsubs& in) & -> strsubs& {
        return base::template operator=(in);
    }

    template<typename t_type>
    template<typename t_self, typename... t_opt>
    constexpr auto strsubs<t_type>::subcs(this t_self& self, t_opt... opt) -> strsubs<apply_qcv<const t_type, t_self>> {
        return strsubs<apply_qcv<const t_type, t_self>>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }

    template<typename t_type>
    template<typename t_self, typename... t_opt>
    constexpr auto strsubs<t_type>::subms(this t_self& self, t_opt... opt) -> strsubs<apply_qcv<t_type, t_self>> {
        return strsubs<apply_qcv<t_type, t_self>>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }
}

#endif