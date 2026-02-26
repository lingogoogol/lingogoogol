#ifndef LGO_BASIC_TYPE_INT_S_ALIAS
#define LGO_BASIC_TYPE_INT_S_ALIAS

#include "decl.hpp"

#include "../p/basic.hpp"
#include "../../tem/t/def.hpp"

LGO_EXPORT namespace lgo {
    using ints8 = ints_class<basic_intp8>;
    using ints8_t = t<ints8>;
    using ints16 = ints_class<basic_intp16>;
    using ints16_t = t<ints16>;
    using ints32 = ints_class<basic_intp32>;
    using ints32_t = t<ints32>;
    using ints64 = ints_class<basic_intp64>;
    using ints64_t = t<ints64>;
    
    using intsc = ints_class<basic_intpc>;
    using intsc_t = t<intsc>;
}

#endif