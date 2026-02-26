#ifndef LGO_BASIC_SEQ_STR_MD_ALIAS
#define LGO_BASIC_SEQ_STR_MD_ALIAS

#include "decl.hpp"

#include "../../../type/cha/alias.hpp"
#include "../../../type/tem/t/def.hpp"

LGO_EXPORT namespace lgo {
    using strmd8 = strmd<cha8>;
    using strmd8_t = t<strmd8>;
    using strmd16 = strmd<cha16>;
    using strmd16_t = t<strmd16>;
    using strmd32 = strmd<cha32>;
    using strmd32_t = t<strmd32>;

    using strmdc = strmd<chac>;
    using strmdc_t = t<strmdc>;
}

#endif