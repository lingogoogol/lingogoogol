#ifndef LGO_BASIC_SEQ_MAIN_D_DEF
#define LGO_BASIC_SEQ_MAIN_D_DEF

#include "cpt.hpp"

#include "../s/def.hpp"

namespace lgo {
    class seqd: public seqs {
    public:
        template<is_seqd_cpt u_self, typename u_in>
        auto push(this u_self& self, u_in&& in) -> u_self&;
        template<is_seqd_cpt u_self, typename u_in>
        auto insert(this u_self& self, intpc pos, const u_in& in) -> u_self&;
        template<is_seqd_cpt u_self, typename u_in>
        auto replace(this u_self& self, intpc pos, intpc len, const u_in& in) -> u_self&;
    };
}

#endif