#ifndef LGO_BASIC_SEQ_IO_D_IMPL
#define LGO_BASIC_SEQ_IO_D_IMPL

#include "def.hpp"

#include "../s/impl.hpp"
#include "../../../io/e/.hpp"

namespace lgo {
    template<typename t_self, typename t_in, typename... t_param>
    auto seqiod::pad_resize(this t_self&& self, const t_in& in, t_param... param) -> t_self&& {
        boo succeed{ U"0"_boo };
        while (!succeed.basic()) {
            try {
                self.pad(in, param...);
                succeed = U"1"_boo;
            }
            catch (pad_not_enough e) {
                self.resize(self.len() + e.shortage());
            }
        }
        return self.f<t_self>();
    }
}

#endif