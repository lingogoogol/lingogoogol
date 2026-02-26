#ifndef LGO_BASIC_SEQ_MAIN_MD_OPT_LEN_DEF
#define LGO_BASIC_SEQ_MAIN_MD_OPT_LEN_DEF

#include "decl.hpp"

#include "../../../../../type/int/p/def.hpp"
#include "../../../../../type/tem/.hpp"
#include "../../../../../type/lgo/.hpp"

LGO_EXPORT namespace lgo {
    class seqmd_len: public lgo {
    public:
        constexpr seqmd_len() = default;
        constexpr seqmd_len(intpc in);

        constexpr auto operator=(const seqmd_len&) & -> seqmd_len& = default;

        constexpr auto len() -> intpc;
    private:
        intpc m_len{};
    };
}

#endif