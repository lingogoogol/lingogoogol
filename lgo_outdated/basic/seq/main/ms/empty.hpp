#ifndef LGO_BASIC_SEQ_MAIN_MS_EMPTY
#define LGO_BASIC_SEQ_MAIN_MS_EMPTY

#include "nonempty_def.hpp"

#include "../s/impl.hpp"
#include "../../../type/int/p/def.hpp"

namespace lgo {
    template<typename t_type>
    class seqms<t_type, intpc::val_v<U"0">>: public seqs {
    public:
        using type = t_type;
        using type_t = t<type>;
        
        constexpr seqms() = default;
    };
}

#endif