#ifndef LGO_BASIC_TYPE_CHA_ALIAS
#define LGO_BASIC_TYPE_CHA_ALIAS

#include "decl.hpp"
#include "basic_decl.hpp"

#include "../tem/t/def.hpp"

LGO_EXPORT namespace lgo {
    using cha8 = cha<basic_cha8>;
    using cha8_t = t<cha8>;
    using cha16 = cha<basic_cha16>;
    using cha16_t = t<cha16>;
    using cha32 = cha<basic_cha32>;
    using cha32_t = t<cha32>;

    using chac = cha<basic_chac>;
    using chac_t = t<chac>;
}

#endif