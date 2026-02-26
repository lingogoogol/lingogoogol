#ifndef LGO_BASIC_FUN_ALGO
#define LGO_BASIC_FUN_ALGO

#include "bind_impl.hpp"
#include "plain_impl.hpp"
#include "cpt.hpp"

#include "../type/int/p/.hpp"
#include "../type/boo/.hpp"
#include "../type/deduce/.hpp"
#include "../type/constraint/.hpp"
#include "../type/fun/.hpp"
#include "../e/.hpp"
#include "../seq/arr/mc/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_in, typename... t_other>
    constexpr auto cond(t_in&& in, t_other&&... other) -> rm_qr<t_in>::template type<U"1"_intpc>::ret
    requires is_arrmc_cpt<rm_qcvr<t_in>> && is_same_cpt<typename rm_qr<t_in>::template type<U"0"_intpc>, boo>
    && is_fun_cpt<typename rm_qr<t_in>::template type<U"1"_intpc>> {
        if (in.ind<0>().basic()) {
            return in.ind<1>()();
        }
        if constexpr (pack_size<t_other...> == U"0"_intpc) {
            throw e_other{ U""_strmdc };
        }
        else {
            return cond(other...);
        }
    }

    template<typename t_in, typename... t_other>
    constexpr auto cond_val(t_in&& in, t_other&&... other) -> rm_qr<t_in>::template type<U"1"_intpc>
    requires is_arrmc_cpt<rm_qcvr<t_in>> && is_same_cpt<typename rm_qr<t_in>::template type<U"0"_intpc>, boo> {
        if (in.ind<0>().basic()) {
            return in.ind<1>();
        }
        if constexpr (pack_size<t_other...> == U"0"_intpc) {
            throw e_other{ U""_strmdc };
        }
        else {
            return cond_val(other...);
        }
    }

    template<typename t_first, typename t_last>
    constexpr auto iter(t_first&& first, t_last&& last) -> void
    requires is_fun_cpt<rm_qcvr<t_first>> && is_fun_cpt<rm_qcvr<t_first>> {
        while (first()) {
            last();
        }
        return;
    }
}

#endif