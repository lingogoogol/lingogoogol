#ifndef LGO_BASIC_SEQ_MAIN_D_IMPL
#define LGO_BASIC_SEQ_MAIN_D_IMPL

#include "def.hpp"

#include "../s/impl.hpp"

namespace lgo {
    template<is_seqd_cpt u_self, typename u_in>
    auto seqd::push(this u_self& self, u_in&& in) -> u_self& {
        self.resize(self.len() + U"1"_intpc);
        self[self.len() - U"1"_intpc] = ::lgo::f<u_in>(in);
        return self;
    }

    template<is_seqd_cpt u_self, typename u_in>
    auto seqd::insert(this u_self& self, intpc pos, const u_in& in) -> u_self& {
        return self.replace(pos, U"0"_intpc, in);
    }

    template<is_seqd_cpt u_self, typename u_in>
    auto seqd::replace(this u_self& self, intpc pos, intpc len, const u_in& in) -> u_self& {
        intpc after{ pos + len };
        u_self temp{ self.subs(after) };
        self.resize(self.len() - len + in.len());
        self.subs(pos, in.len()) = in.subs();
        self.subs(pos + in.len()) = temp;
        return self;
    }
}

#endif