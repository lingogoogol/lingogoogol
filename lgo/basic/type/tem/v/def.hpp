#ifndef LGO_BASIC_TYPE_TEM_V_DEF
#define LGO_BASIC_TYPE_TEM_V_DEF

#include "../t/def.hpp"

#include "../../deduce/qualifier_rm.hpp"
#include "../../lgo/def.hpp"

LGO_EXPORT namespace lgo {
    template<const auto& t_val>
    class v: public lgo {
    public:
        v() = delete;
        ~v() = delete;

        using type = rm_qcvr<decltype(t_val)>;
        using type_t = t<type>;

        constexpr static type val{ t_val };
    };

    template<const auto& t_val>
    using v_t = t<v<t_val>>;
}

#endif