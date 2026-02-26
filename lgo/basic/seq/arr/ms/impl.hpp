#ifndef LGO_BASIC_SEQ_ARR_MS_IMPL
#define LGO_BASIC_SEQ_ARR_MS_IMPL

#include "def.hpp"

#include "../../main/ms/nonempty_impl.hpp"

namespace lgo {
    template<typename t_type, is_intp_v_cpt t_len>
    constexpr auto arrms<t_type, t_len>::operator=(const arrms& in) & -> arrms& {
        return base::template operator=(in);
    }
}

#endif