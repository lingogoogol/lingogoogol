#ifndef LGO_BASIC_TYPE_FUN_CMP
#define LGO_BASIC_TYPE_FUN_CMP

#include "../boo/.hpp"
#include "../constraint/cmp.hpp"
#include "../constraint/common.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in1, typename t_in2>
    constexpr auto operator==(t_in1&& in1, t_in2&& in2) -> boo
    requires is_comparable_cpt<t_in1, t_in2> {
        return boo{ (in1 <=> in2).basic() == 0 };
    }

    template<typename t_in1, typename t_in2>
    constexpr auto operator!=(t_in1&& in1, t_in2&& in2) -> boo
    requires requires () {
        { in1 == in2 } -> is_same_cpt<boo>;
    } {
        return !(in1 == in2);
    }

    template<typename t_in1, typename t_in2>
    constexpr auto operator<(t_in1&& in1, t_in2&& in2) -> boo
    requires is_comparable_cpt<t_in1, t_in2> {
        return boo{ (in1 <=> in2).basic() < 0 };
    }

    template<typename t_in1, typename t_in2>
    constexpr auto operator>(t_in1&& in1, t_in2&& in2) -> boo
    requires is_comparable_cpt<t_in1, t_in2> {
        return boo{ (in1 <=> in2).basic() > 0 };
    }

    template<typename t_in1, typename t_in2>
    constexpr auto operator<=(t_in1&& in1, t_in2&& in2) -> boo
    requires is_comparable_cpt<t_in1, t_in2> {
        return boo{ (in1 <=> in2).basic() <= 0 };
    }

    template<typename t_in1, typename t_in2>
    constexpr auto operator>=(t_in1&& in1, t_in2&& in2) -> boo
    requires is_comparable_cpt<t_in1, t_in2> {
        return boo{ (in1 <=> in2).basic() >= 0 };
    }
}

#endif