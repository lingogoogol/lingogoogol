#ifndef LGO_BASIC_TYPE_LGO_DEF
#define LGO_BASIC_TYPE_LGO_DEF

#include "../deduce/qualifier_rm.hpp"
#include "../deduce/qualifier_apply_basic.hpp"
#include "../ptr/s/decl.hpp"

LGO_EXPORT namespace lgo {
    class lgo {
    public:
        template<typename t_out, typename t_self>
        constexpr auto sc(this t_self&& self) -> t_out;
        template<typename t_self>
        constexpr auto r(this t_self&& self) -> rm_qr<t_self>&&;
        template<typename t_par, typename t_self>
        constexpr auto f(this t_self&& self) -> apply_qr_basic<rm_qr<t_self>, t_par&&>;
        template<typename t_self>
        constexpr auto operator&(this t_self&& self) -> ptrs<t_self>;
    };
}

#endif