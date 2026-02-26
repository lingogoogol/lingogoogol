#ifndef LGO_BASIC_SEQ_ARR_MC_MAIN_ALIAS
#define LGO_BASIC_SEQ_ARR_MC_MAIN_ALIAS

#include "decl.hpp"

#include "../../../../type/tem/.hpp"

LGO_EXPORT namespace lgo {
    template<typename... t_type>
    using arrmc_t = t<arrmc<t_type...>>;
}

#endif