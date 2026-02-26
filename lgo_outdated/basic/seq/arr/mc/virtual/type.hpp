#ifndef LGO_BASIC_SEQ_ARR_MC_VIRTUAL_TYPE
#define LGO_BASIC_SEQ_ARR_MC_VIRTUAL_TYPE

#include "cvt.hpp"
#include "merge.hpp"

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_type_private: private arrmc_cvt_private<arrmc_type_private<t_disambiguater, t_cvt, t_type...>, t_cvt>
    , private arrmc_tmerge_private<arrmc_type_private<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base_cvt = arrmc_cvt_private<arrmc_type_private, t_cvt>;
        using base_tmerge = arrmc_tmerge_private<arrmc_type_private, t_cvt, t_type...>;

        template<is_intp_v_cpt, typename...>
        struct type_impl_stu {};

        template<is_intp_v_cpt t_pos, typename u_type, typename... u_other>
        struct type_impl_stu<t_pos, u_type, u_other...> {
            constexpr static intpc next_pos{ t_pos::val - U"1"_intpc };
            using type = type_impl_stu<v<next_pos>, u_other...>::type;
        };

        template<is_intp_v_cpt t_pos, typename u_type, typename... u_other>
        requires ((t_pos::val == U"0"_intpc).cpt())
        struct type_impl_stu<t_pos, u_type, u_other...> {
            using type = u_type;
        };

        template<is_intp_v_cpt t_pos>
        struct type_stu {
            using type = type_impl_stu<t_pos, t_type...>::type;
        };

        template<is_intp_v_cpt t_pos, is_intp_v_cpt t_len, typename... u_type>
        struct type_sub_impl_stu {};

        template<is_intp_v_cpt t_pos, is_intp_v_cpt t_len, typename u_type, typename... u_other>
        struct type_sub_impl_stu<t_pos, t_len, u_type, u_other...> {
            constexpr static intpc next_pos{ t_pos::val - U"1"_intpc };
            using type = type_sub_impl_stu<v<next_pos>, t_len, u_other...>::type;
        };

        template<is_intp_v_cpt t_pos, is_intp_v_cpt t_len, typename u_type, typename... u_other>
        requires ((t_pos::val == U"0"_intpc).cpt())
        struct type_sub_impl_stu<t_pos, t_len, u_type, u_other...> {
            template<typename>
            struct merge {};
            template<typename... v_returned>
            struct merge<arrmc_base<v_returned...>> {
                using type = arrmc_base<u_type, v_returned...>;
            };
            
            constexpr static intpc next_len{ t_len::val - U"1"_intpc };
            using type = merge<typename type_sub_impl_stu<intpc::val_v<U"0">, v<next_len>, u_other...>::type>::type;
        };

        template<is_intp_v_cpt t_pos, is_intp_v_cpt t_len, typename... u_type>
        requires ((t_pos::val == U"0"_intpc && t_len::val == U"0"_intpc).cpt())
        struct type_sub_impl_stu<t_pos, t_len, u_type...> {
            using type = arrmc_base<>;
        };

        template<is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
        struct type_sub_stu {
            using type = base_cvt::template from_base_private<type_sub_impl_stu<t_pos, t_len, t_type...>::type>;
        };
    protected:
        using type_bind_private = bind<tem<type_stu>>;
        template<is_intp_v_cpt t_pos>
        using type_private = type_stu<t_pos>::type;
        using type_sub_bind_private = bind<tem<type_sub_stu>>;
        template<is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
        using type_sub_private = type_sub_stu<t_pos, t_len>::type;
    };

    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_type: private arrmc_type_private<arrmc_type<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base = arrmc_type_private<arrmc_type, t_cvt, t_type...>;
    public:
        using type_bind = base::type_bind_private;
        template<is_intp_v_cpt t_pos>
        using type = base::template type_private<t_pos>;
        using type_sub_bind = base::type_sub_bind_private;
        template<is_intp_v_cpt t_pos, is_intp_v_cpt t_len>
        using type_sub = base::template type_sub_private<t_pos, t_len>;
    };
}

#endif