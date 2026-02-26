#ifndef LGO_BASIC_SEQ_MAIN_MD_OPT_RESERVE_IMPL
#define LGO_BASIC_SEQ_MAIN_MD_OPT_RESERVE_IMPL

#include "def.hpp"

#include "../../../../../type/int/p/impl.hpp"

namespace lgo {
    constexpr seqmd_reserve::seqmd_reserve(intpc reserve): m_reserve{ reserve } {}

    constexpr auto seqmd_reserve::reserve() -> intpc {
        return m_reserve;
    }
}

#endif