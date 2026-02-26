#ifndef LGO_BASIC_SEQ_MAIN_C_IMPL
#define LGO_BASIC_SEQ_MAIN_C_IMPL

#include "def.hpp"

#include "../../../type/int/p/literal.hpp"

namespace lgo {
    template<typename t_self, is_fun_cpt t_fun>
    constexpr auto seqc::each(this t_self&& self, const t_fun& fun) -> t_self&& {
        if constexpr (t_self::len() != 0) {
            fun(self.template ind<intpc::val_v<U"0">>());
            self.template sub<intpc::val_v<U"1">, t_self::len() - U"1"_intpc>().each(fun);
        }
        return self.f<t_self>();
    }
}

#endif