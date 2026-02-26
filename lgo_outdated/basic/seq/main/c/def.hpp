#ifndef LGO_BASIC_SEQ_MAIN_C_DEF
#define LGO_BASIC_SEQ_MAIN_C_DEF

#include "../../../fun/cpt.hpp"

namespace lgo {
    class seqc {
    public:
        template<typename t_self, is_fun_cpt t_fun>
        constexpr auto each(this t_self&& self, const t_fun& fun) -> t_self&&;
    };
}

#endif