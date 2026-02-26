#ifndef LGO_BASIC_SEQ_MAIN_S_DEF
#define LGO_BASIC_SEQ_MAIN_S_DEF

#include "../c/def.hpp"
#include "../../../fun/cpt.hpp"
#include "../../../type/int/p/alias.hpp"

namespace lgo {
    class seqs: public seqc {
    public:
        template<typename t_self, typename t_in>
		constexpr auto operator==(this const t_self& self, const t_in& in) -> boo;

        template<typename t_self, is_fun_cpt t_fun>
        constexpr auto each(this t_self&& self, const t_fun& fun) -> void;
        template<typename t_self, is_fun_cpt t_fun>
        constexpr auto find(this t_self&& self, const t_fun& fun) -> intpc;
    };
}

#endif