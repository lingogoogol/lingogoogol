#ifndef LGO_BASIC_SEQ_ARR_SUBD_ALIAS
#define LGO_BASIC_SEQ_ARR_SUBD_ALIAS

#include "decl.hpp"

LGO_EXPORT namespace lgo {
    template<typename... t_in>
    using arrsubd_t = t<arrsubd<t_in...>>;
    using arrsubd_bind = bind<tem<arrsubd>>;
}

#endif