#ifndef LGO_BASIC_IO_OPT_LEN_IMPL
#define LGO_BASIC_IO_OPT_LEN_IMPL

#include "def.hpp"

namespace lgo {
    constexpr io_len::io_len(intpc len): m_len{ len } {}

    constexpr auto io_len::len() const -> intpc {
        return m_len;
    }
}

#endif