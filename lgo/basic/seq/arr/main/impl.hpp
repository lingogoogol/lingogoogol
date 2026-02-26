#ifndef LGO_BASIC_SEQ_ARR_MAIN_IMPL
#define LGO_BASIC_SEQ_ARR_MAIN_IMPL

#include "def.hpp"

#include "../../../bind/then.hpp"
#include "../../../bind/type.hpp"
#include "../../../bind/cond.hpp"
#include "../mc/main/def.hpp"

namespace lgo {
    template<typename t_opt_arr>
    constexpr auto arr::get_for_all(t_opt_arr&& opt_arr, for_all_type<rm_qr<t_opt_arr>>& out) -> void {
        if constexpr (contain_for_all<rm_qr<t_opt_arr>>.basic()) {
            out = opt_arr.template ind<typename rm_qr<t_opt_arr>::template tfind_one<is_arr_for_all_bind>>();
        }
        return;
    }

    template<is_arr_cpt t_self, typename t_opt>
    constexpr auto arr::size(this const t_self& self, t_opt&& opt) -> intpc
    requires is_size_mem_cpt<rm_qr<t_opt>> {
        using opt_type = rm_qr<t_opt>;
        intpc out{ U"0"_intpc };

        if constexpr (false) {
            "seq_len";
            throw e_other{ U"not implemented"_strmdc };
        }
        else if constexpr ((!opt_type::template tcontain<bind<is_same_bind, bind_arg<U"0">, seq_plain>>).basic()) {
            "smart select";
            throw e_other{ U"not implemented"_strmdc };
        }

        for_all_type<opt_type> for_all{};
        get_for_all(opt, for_all);

        constexpr static fun size_fun{
            [] <typename u_element, typename... u_in> (u_element&& element, u_in&&... in) -> intpc {
                return ::lgo::f<u_element>(element).size(size_mem{ ::lgo::f<u_in>(in)... });
            }
        };

        for (intpc i{ U"0"_intpc }; (i < self.len()).basic(); ++i) {
            out += arr_for_all{ self[i] }.merge(for_all).r().apply(size_fun);
        }
        return out;
    }

    template<is_arrs_cpt t_in, is_mem_cpt t_out, typename... t_opt>
    constexpr auto arr::mem_pad(const t_in& in, t_out& out, t_opt&&... opt) -> void
    requires (is_arr_opt_mem_pad_cpt<rm_qr<t_opt>> && ...) {
        using opt_arr_type = arrmc<t_opt...>;
        opt_arr_type opt_arr{ seq_data{ opt... } };

        if constexpr (false) {
            "seq_len";
            throw e_other{ U"not implemented"_strmdc };
        }
        else if constexpr ((!opt_arr_type::template tcontain<bind<is_same_bind, bind_arg<U"0">, seq_plain>>).basic()) {
            "smart select";
            throw e_other{ U"not implemented"_strmdc };
        }

        for_all_type<opt_arr_type> for_all{};
        get_for_all(opt_arr, for_all);

        constexpr static fun pad_fun{
            [] <typename... u_in> (u_in&&... in) {
                mem_pad_fun_selector<typename t_in::type, t_out&, for_all_type<opt_arr_type>>::val(::lgo::f<u_in>(in)...);
            }
        };

        constexpr static boo inverse{ opt_arr_type::template tcontain<bind<is_same_bind, bind_arg<U"0">, seq_inverse>> };
        for (intpc i{ inverse.basic() ? (in.len() - U"1"_intpc).sc<intpc>() : U"0"_intpc }
        ; inverse.basic() ? (i < intpc::max()).basic() : (i < in.len()).basic(); inverse.basic() ? --i : ++i) {
            arr_for_all{ out }.merge(arr_for_all{ in[i] }, for_all).r().apply(pad_fun);
        }
        return;
    }
}

#endif