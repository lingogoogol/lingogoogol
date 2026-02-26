#ifndef LGO_BASIC_TYPE_TEM_BIND_ARG
#define LGO_BASIC_TYPE_TEM_BIND_ARG

#include "../../literal_str/basic.hpp"
#include "../../lgo/def.hpp"

LGO_EXPORT namespace lgo {
    template<basic_literal_str u_pos>
    class bind_arg: public lgo {
    public:
        bind_arg() = delete;
        ~bind_arg() = delete;
    };
}

#endif