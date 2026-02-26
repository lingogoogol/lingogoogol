#ifndef LGO_BASIC_SEQ_MAIN_MD_OPT_RESERVE_DEF
#define LGO_BASIC_SEQ_MAIN_MD_OPT_RESERVE_DEF

#include "decl.hpp"

#include "../../../../../type/int/p/def.hpp"
#include "../../../../../type/tem/.hpp"
#include "../../../../../type/lgo/.hpp"

LGO_EXPORT namespace lgo {
    class seqmd_reserve: public lgo {
    public:
        constexpr seqmd_reserve() = default;
        constexpr seqmd_reserve(intpc reserve);
        
        constexpr auto operator=(const seqmd_reserve&) & -> seqmd_reserve& = default;

        constexpr auto reserve() -> intpc;
    private:
        intpc m_reserve{};
    };
}

#endif