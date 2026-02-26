#ifndef LGO_BASIC_TYPE_TEM_T_DEF
#define LGO_BASIC_TYPE_TEM_T_DEF

#include "decl.hpp"

#include "../../lgo/def.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_type>
    class t: public lgo {
    public:
        t() = delete;
        ~t() = delete;

        using type = t_type;
        using type_t = t<type>;
    };
}

#endif