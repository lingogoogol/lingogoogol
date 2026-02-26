#ifndef LGO_BASIC_SEQ_STR_MD_IMPL
#define LGO_BASIC_SEQ_STR_MD_IMPL

#include "def.hpp"

#include "../main/impl.hpp"
#include "../subd/impl.hpp"
#include "../../main/md/impl.hpp"
#include "../../io/d/impl.hpp"

namespace lgo {
    template<is_cha_cpt t_type>
    template<basic_literal_str u_in>
    auto strmd<t_type>::make() -> strmd {
        strmd out{ seqmd_len{ ::lgo::sc<intpc>(u_in.len()) } };
        for (basic_intpc i{ 0 }; i < u_in.len(); ++i) {
            out[::lgo::sc<intpc>(i)] = ::lgo::sc<chac>(u_in[i]);
        }
        return out;
    }

    template<is_cha_cpt t_type>
    strmd<t_type>::strmd(const strmd& in): base{ in } {
        base::ctor(in);
        return;
    }

    template<is_cha_cpt t_type>
    template<is_seq_impl_cpt t_in>
    strmd<t_type>::strmd(const t_in& in)
    requires is_same_cpt<typename t_in::type, typename base::type>: base{ in } {
        base::ctor(in);
        return;
    }
    
    template<is_cha_cpt t_type>
    auto strmd<t_type>::operator=(const strmd& in) & -> strmd& {
        return base::template operator=(in);
    }
    
    template<is_cha_cpt t_type>
    auto strmd<t_type>::operator=(strmd&& in) & -> strmd& {
        return base::template operator=(in.r());
    }

    template<is_cha_cpt t_type>
    template<typename t_self, typename... t_opt>
    auto strmd<t_type>::subcs(this t_self& self, t_opt... opt) -> strsubs<const t_self> {
        return strsubs<const t_self>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }

    template<is_cha_cpt t_type>
    template<typename t_self, typename... t_opt>
    auto strmd<t_type>::subms(this t_self& self, t_opt... opt) -> strsubs<t_self> {
        return strsubs<t_self>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }

    template<is_cha_cpt t_type>
    template<typename t_self, typename... t_opt>
    auto strmd<t_type>::subcd(this t_self& self, t_opt... opt) -> strsubd<const t_self> {
        return strsubd<const t_self>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }

    template<is_cha_cpt t_type>
    template<typename t_self, typename... t_opt>
    auto strmd<t_type>::submd(this t_self& self, t_opt... opt) -> strsubd<t_self> {
        return strsubd<t_self>{ self, opt..., placeholder<intpc::val_v<U"0">>{} };
    }
}

#endif