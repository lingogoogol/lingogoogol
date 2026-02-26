#ifndef LGO_BASIC_SEQ_MAIN_MD_OPT_LEN_IMPL
#define LGO_BASIC_SEQ_MAIN_MD_OPT_LEN_IMPL

#include "def.hpp"

#include "../../../../../type/int/p/impl.hpp"

namespace lgo {
    constexpr seqmd_len::seqmd_len(intpc len): m_len{ len } {}

    constexpr auto seqmd_len::len() -> intpc {
        return m_len;
    }
}

#endif