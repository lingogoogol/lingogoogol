#ifndef LGO_BASIC_SEQ_ARR_MC_VIRTUAL_REDUCE
#define LGO_BASIC_SEQ_ARR_MC_VIRTUAL_REDUCE

#include "len.hpp"

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_treduce_private
    : private arrmc_len_private<arrmc_treduce_private<t_disambiguater, t_cvt, t_type...>, t_type...> {
    private:
        template<is_bind_cpt t_bind, typename t_previous, is_intp_v_cpt t_pos
        , typename t_state, typename... u_other>
        struct treduce_stu {};

        template<is_bind_cpt t_bind, typename t_previous, is_intp_v_cpt t_pos
        , typename t_state, typename u_type, typename... u_other>
        struct treduce_stu<t_bind, t_previous, t_pos, t_state, u_type, u_other...> {
            constexpr static intpc next_pos{ t_pos::val + U"1"_intpc };
            using stu = typename t_bind::template type<t_previous, u_type, t_pos, t_state>;
            using type = treduce_stu<t_bind, typename stu::processed, v<next_pos>, typename stu::state, u_other...>::type;
        };

        template<is_bind_cpt t_bind, typename t_previous, is_intp_v_cpt t_pos, typename t_state>
        struct treduce_stu<t_bind, t_previous, t_pos, t_state> {
            using type = t_previous;
        };
    protected:
        template<is_bind_cpt u_bind, typename u_start, typename u_state>
        using treduce_private = treduce_stu<u_bind, u_start, intpc::val_v<U"0">, u_state, t_type...>::type;
    };
    
    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_treduce: private arrmc_treduce_private<arrmc_treduce<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base = arrmc_treduce_private<arrmc_treduce, t_cvt, t_type...>;
    public:
        template<is_bind_cpt u_bind, typename u_start, typename u_state>
        using treduce = base::template treduce_private<u_bind, u_start, u_state>;
    };
}

#endif