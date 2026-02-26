#ifndef LGO_BASIC_TYPE_INT_P_ALIAS
#define LGO_BASIC_TYPE_INT_P_ALIAS

#include "decl.hpp"
#include "basic.hpp"

#include "../../tem/t/decl.hpp"

LGO_EXPORT namespace lgo {
    using intp8 = intp_class<basic_intp8>;
    using intp8_t = t<intp8>;
    using intp16 = intp_class<basic_intp16>;
    using intp16_t = t<intp16>;
    using intp32 = intp_class<basic_intp32>;
    using intp32_t = t<intp32>;
    using intp64 = intp_class<basic_intp64>;
    using intp64_t = t<intp64>;
    
    using intpc = intp_class<basic_intpc>;
    using intpc_t = t<intpc>;

    using byte = intp8;
}

#endif