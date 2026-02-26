#ifndef LGO_BASIC_TYPE_INT_N_ALIAS
#define LGO_BASIC_TYPE_INT_N_ALIAS

#include "decl.hpp"

#include "../p/basic.hpp"
#include "../../tem/t/def.hpp"

LGO_EXPORT namespace lgo {
    using intn8 = intn_class<basic_intp8>;
    using intn8_t = t<intn8>;
    using intn16 = intn_class<basic_intp16>;
    using intn16_t = t<intn16>;
    using intn32 = intn_class<basic_intp32>;
    using intn32_t = t<intn32>;
    using intn64 = intn_class<basic_intp64>;
    using intn64_t = t<intn64>;

    using intnc = intn_class<basic_intpc>;
    using intnc_t = t<intnc>;
}

#endif