#ifndef LGO_BASIC_IO_E_GET_NOT_ENOUGH
#define LGO_BASIC_IO_E_GET_NOT_ENOUGH

#include "../../seq/str/md/literal.hpp"
#include "../../e/.hpp"

LGO_EXPORT namespace lgo {
    class get_not_enough: public lgo, public e {
    public:
        get_not_enough(intpc shortage);

        auto shortage() -> intpc;
    private:
        intpc m_shortage{};
    };

    get_not_enough::get_not_enough(intpc shortage): e{ U""_strmdc }, m_shortage{ shortage } {}

    auto get_not_enough::shortage() -> intpc {
        return m_shortage;
    }
}

#endif