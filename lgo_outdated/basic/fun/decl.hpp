#ifndef LGO_BASIC_FUN_DECL
#define LGO_BASIC_FUN_DECL

#include "arg/placeholder/def.hpp"

#include "../type/tem/t/.hpp"
#include "../seq/arr/mc/main/decl.hpp"

namespace lgo {
    template<typename t_type, is_arrmc_cpt t_arg>
    class fun;
    template<typename t_type>
    class fun<t_type, arrmc<fun_placeholder_other>>;
}

#endif