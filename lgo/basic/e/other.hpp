#ifndef LGO_BASIC_E_OTHER
#define LGO_BASIC_E_OTHER

#include "impl.hpp"

#include "../seq/str/md/literal.hpp"

LGO_EXPORT namespace lgo {
    class e_other: public lgo, public e {
    public:
        e_other(strmdc message);
    };

    e_other::e_other(strmdc message): e{ message } {}
}

#endif