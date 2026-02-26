#ifndef LGO_BASIC_TYPE_LGO_IMPL
#define LGO_BASIC_TYPE_LGO_IMPL

#include "def.hpp"

#include "../fun/cast_impl.hpp"
#include "../ptr/s/decl.hpp"

namespace lgo {
    template<typename t_out, typename t_self>
    constexpr auto lgo::sc(this t_self&& self) -> t_out {
        return ::lgo::sc<t_out>(::lgo::sc<t_self&&>(self));
    }

    template<typename t_self>
    constexpr auto lgo::r(this t_self&& self) -> rm_qr<t_self>&& {
        return ::lgo::r(self);
    }

    template<typename t_par, typename t_self>
    constexpr auto lgo::f(this t_self&& self) -> apply_qr_basic<rm_qr<t_self>, t_par&&> {
        return ::lgo::f<t_par>(self);
    }

    template<typename t_self>
    constexpr auto lgo::operator&(this t_self&& self) -> ptrs<t_self> {
        return ptrs<t_self>{ std::addressof(self) };
    }
}

#endif