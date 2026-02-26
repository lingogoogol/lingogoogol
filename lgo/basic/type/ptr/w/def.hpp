#ifndef LGO_BASIC_TYPE_PTR_W_DEF
#define LGO_BASIC_TYPE_PTR_W_DEF

#include "decl.hpp"

#include "../main/def.hpp"
#include "../../lgo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_type>
    class ptrw: public lgo, public ptr<t_type> {
    public:
    private:
    };
}

#endif