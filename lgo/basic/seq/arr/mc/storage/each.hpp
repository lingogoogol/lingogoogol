#ifndef LGO_BASIC_SEQ_ARR_MC_STORAGE_EACH
#define LGO_BASIC_SEQ_ARR_MC_STORAGE_EACH

#include "../virtual/each.hpp"

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_each_private
    : private arrmc_cvt_private<arrmc_each_private<t_disambiguater, t_cvt, t_type...>, t_cvt>
    , private arrmc_teach_private<arrmc_each_private<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base_cvt = arrmc_cvt_private<arrmc_each_private, t_cvt>;
        using base_teach = arrmc_teach_private<arrmc_each_private, t_cvt, t_type...>;

        template<is_intp_v_cpt t_ind, typename t_self, is_fun_cpt t_fun, typename t_out>
        constexpr auto each_impl(this t_self&& self, const t_fun& fun, t_out& out) -> void;
        template<is_intp_v_cpt t_ind, typename t_self, is_fun_cpt t_fun, typename t_out>
        constexpr static fun each_impl_fun{ &each_impl<t_ind, t_self, t_fun, t_out> };

        template<typename t_fun>
        struct each_out_type_stu {
            template<typename t_in>
            struct bind_stu {
                using type = decltype(declval<t_fun>()(t_in));
            };
        };
    protected:
        template<typename t_self, is_fun_cpt t_fun>
        constexpr auto each_private(this t_self&& self, const t_fun& fun) -> auto;
        template<typename t_self, is_fun_cpt t_fun>
        constexpr static fun each_private_fun{ &each_private<t_self, t_fun> };
    };

    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    template<is_intp_v_cpt t_ind, typename t_self, is_fun_cpt t_fun, typename t_out>
    constexpr auto arrmc_each_private<t_disambiguater, t_cvt, t_type...>
    ::each_impl(this t_self&& self, const t_fun& fun, t_out& out) -> void {
        out.ind<t_ind>() = fun(::lgo::f<t_self>(self.ind<t_ind>()));
        constexpr static intpc next_ind{ t_ind::val + U"1"_intpc };
        if constexpr ((next_ind < t_self::len()).basic()) {
            each_impl_fun<v<next_ind>, t_self, t_fun, t_out>(::lgo::f<t_self>(self), fun, out);
        }
        return;
    }

    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    template<typename t_self, is_fun_cpt t_fun>
    constexpr auto arrmc_each_private<t_disambiguater, t_cvt, t_type...>
    ::each_private(this t_self&& self, const t_fun& fun) -> auto {
        using out_type = base_teach::template teach_private<bind<tem<each_out_type_stu<t_fun>::bind_stu>>>;
        out_type out{};
        each_impl_fun<intpc::val_v<U"0">, t_self, t_fun, out_type>(::lgo::f<t_self>(self), fun, out);
        return out;
    }

    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_each: private arrmc_each_private<arrmc_each<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base = arrmc_each_private<arrmc_each, t_cvt>;
    public:
        template<typename t_self, is_fun_cpt t_fun>
        constexpr auto each(this t_self&& self, const t_fun& fun) -> auto;
    };

    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    template<typename t_self, is_fun_cpt t_fun>
    constexpr auto arrmc_each<t_disambiguater, t_cvt, t_type...>::each(this t_self&& self, const t_fun& fun) -> auto {
        return base::template each_private_fun<t_self, t_fun>(::lgo::f<t_self>(self), fun);
    }
}

#endif