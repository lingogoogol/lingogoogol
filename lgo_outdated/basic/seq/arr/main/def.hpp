#ifndef LGO_BASIC_SEQ_ARR_MAIN_DEF
#define LGO_BASIC_SEQ_ARR_MAIN_DEF

#include "opt/cpt/.hpp"

#include "../cpt/main.hpp"
#include "../../../fun/bind_def.hpp"
#include "../../../fun/plain_impl.hpp"

namespace lgo {
    class arr {
    private:
        template<typename t_opt_arr>
        constexpr static boo contain_for_all{ t_opt_arr::template tcontain<is_arr_for_all_bind> };
        template<typename t_opt_arr>
        using for_all_type = bind_cond<arrmc<v<contain_for_all<t_opt_arr>>, then_bind<typename t_opt_arr::tfind_one_bind
        , typename t_opt_arr::type_bind>, arrmc<is_arr_for_all_bind>>
        , arrmc<boo::val_v<U"1">, iden_bind, arrmc<arr_for_all<>>>>;
        template<typename t_opt_arr>
        constexpr static auto get_for_all(t_opt_arr&& opt_arr, for_all_type<rm_qr<t_opt_arr>>& out) -> void;
        
        template<typename, typename, is_arr_for_all_cpt>
        struct mem_pad_fun_selector {};
        template<typename t_in, typename t_out, typename... t_param>
        struct mem_pad_fun_selector<t_in, t_out, arr_for_all<t_param...>> {
            static inline fun val{ &rm_qr<t_out>::template pad<t_out, t_in, t_param...> };
        };
    public:
		template<is_arr_cpt t_self, typename t_opt>
        constexpr auto size(this const t_self& self, t_opt&& opt) -> intpc
        requires is_size_mem_cpt<rm_qr<t_opt>>;

        template<is_arrs_cpt t_in, is_mem_cpt t_out, typename... t_opt>
        constexpr static auto mem_pad(const t_in& in, t_out& out, t_opt&&... opt) -> void
        requires (is_arr_opt_mem_pad_cpt<rm_qr<t_opt>> && ...);
    };
}

#endif