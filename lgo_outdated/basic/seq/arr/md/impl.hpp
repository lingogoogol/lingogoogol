#ifndef LGO_BASIC_SEQ_ARR_MD_IMPL
#define LGO_BASIC_SEQ_ARR_MD_IMPL

#include "def.hpp"

#include "../subd/impl.hpp"
#include "../../main/md/impl.hpp"

namespace lgo {
    template<typename t_type>
    arrmd<t_type>::arrmd(const arrmd& in): base{ in } {
        base::ctor(in);
        return;
    }

    template<typename t_type>
    template<is_seq_impl_cpt t_in>
    arrmd<t_type>::arrmd(const t_in& in)
    requires is_same_cpt<typename t_in::type, typename base::type>: base{ in } {
        ctor(in);
        return;
    }
    
    template<typename t_type>
    auto arrmd<t_type>::operator=(const arrmd& in) & -> arrmd& {
        return base::template operator=(in);
    }

    template<typename t_type>
    template<typename... t_opt>
    auto arrmd<t_type>::subms(t_opt... opt) -> arrsubs<arrmd> {
        return arrsubs<arrmd>{ *this, opt..., placeholder<intpc::val_v<U"0">>{} };
    }
}

#endif