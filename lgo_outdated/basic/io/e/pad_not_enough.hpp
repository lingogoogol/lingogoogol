#ifndef LGO_BASIC_IO_E_PAD_NOT_ENOUGH
#define LGO_BASIC_IO_E_PAD_NOT_ENOUGH

#include "../../seq/str/md/literal.hpp"
#include "../../e/.hpp"

LGO_EXPORT namespace lgo {
    class pad_not_enough: public lgo, public e {
    public:
        pad_not_enough(intpc shortage);

        auto shortage() -> intpc;
    private:
        intpc m_shortage{};
    };

    pad_not_enough::pad_not_enough(intpc shortage): e{ U""_strmdc }, m_shortage{ shortage } {}

    auto pad_not_enough::shortage() -> intpc {
        return m_shortage;
    }
}

#endif