#ifndef LGO_BASIC_IO_OPT_UNTIL_IGNORE_IMPL
#define LGO_BASIC_IO_OPT_UNTIL_IGNORE_IMPL

#include "def.hpp"

namespace lgo {
    template<is_seqio_cpt t_type>
    io_until_ignore<t_type>::io_until_ignore(t_type delim): m_delim{ delim } {}

    template<is_seqio_cpt t_type>
    auto io_until_ignore<t_type>::delim() const -> const t_type& {
        return m_delim;
    }
}

#endif