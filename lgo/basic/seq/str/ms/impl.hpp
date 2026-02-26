#ifndef LGO_BASIC_SEQ_STR_MS_IMPL
#define LGO_BASIC_SEQ_STR_MS_IMPL

#include "def.hpp"

#include "../../str/main/impl.hpp"
#include "../../io/s/impl.hpp"

namespace lgo {
    template<is_cha_cpt t_type, is_intp_v_cpt t_len>
    template<is_literal_str_cpt t_in>
    constexpr strms<t_type, t_len>::strms(const t_in& in) {
        ctor(in);
    }

    template<is_cha_cpt t_type, is_intp_v_cpt t_len>
    constexpr auto strms<t_type, t_len>::operator=(const strms& in) & -> strms& {
        return base::template operator=(in);
    }

    template<is_cha_cpt t_type, is_intp_v_cpt t_len>
    template<typename t_self, typename... t_opt>
    constexpr auto strms<t_type, t_len>::subcs(this t_self& self, t_opt... opt) -> strsubs<const t_self> {
        return strsubs<const t_self>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }

    template<is_cha_cpt t_type, is_intp_v_cpt t_len>
    template<typename t_self, typename... t_opt>
    constexpr auto strms<t_type, t_len>::subms(this t_self& self, t_opt... opt) -> strsubs<t_self> {
        return strsubs<t_self>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }
}

#endif