#ifndef LGO_BASIC_SEQ_ARR_MC_STORAGE_APPLY
#define LGO_BASIC_SEQ_ARR_MC_STORAGE_APPLY

namespace lgo {
    template<typename t_disambiguater>
    class arrmc_apply_private {
    private:
        template<is_intp_v_cpt t_pos, typename t_self, is_fun_cpt t_fun, typename... t_arg>
        constexpr auto apply_impl(this t_self&& self, const t_fun& fun, t_arg&&... arg) -> auto;
        template<is_intp_v_cpt t_pos, typename t_self, is_fun_cpt t_fun, typename... t_arg>
        constexpr static fun apply_impl_fun{ &apply_impl<t_pos, t_self, t_fun, t_arg...> };
    protected:
        template<typename t_self, is_fun_cpt t_fun>
        constexpr auto apply_private(this t_self&& self, const t_fun& fun) -> auto;
        template<typename t_self, is_fun_cpt t_fun>
        constexpr static fun apply_private_fun{ &apply_private<t_self, t_fun> };
    };
    
    template<typename t_disambiguater>
    template<is_intp_v_cpt t_pos, typename t_self, is_fun_cpt t_fun, typename... t_arg>
    constexpr auto arrmc_apply_private<t_disambiguater>
    ::apply_impl(this t_self&& self, const t_fun& fun, t_arg&&... arg) -> auto {
        if constexpr ((t_pos::val != rm_qr<t_self>::len()).basic()) {
            constexpr static intpc next_pos{ t_pos::val + U"1"_intpc };
            return apply_impl_fun<v<next_pos>, t_self, t_fun, t_arg..., decltype(self.template indf<t_pos, t_self>())>
            (::lgo::f<t_self>(self), fun, ::lgo::f<t_arg>(arg)..., self.template indf<t_pos, t_self>());
        }
        else {
            return fun(::lgo::f<t_arg>(arg)...);
        }
    }

    template<typename t_disambiguater>
    template<typename t_self, is_fun_cpt t_fun>
    constexpr auto arrmc_apply_private<t_disambiguater>::apply_private(this t_self&& self, const t_fun& fun) -> auto {
        return apply_impl_fun<intpc::val_v<U"0">, t_self, t_fun>(::lgo::f<t_self>(self), fun);
    }

    template<typename t_disambiguater>
    class arrmc_apply: private arrmc_apply_private<arrmc_apply<t_disambiguater>> {
    private:
        using base = arrmc_apply_private<arrmc_apply>;
    public:
        template<typename t_self, is_fun_cpt t_fun>
        constexpr auto apply(this t_self&& self, const t_fun& fun) -> auto;
    };

    template<typename t_disambiguater>
    template<typename t_self, is_fun_cpt t_fun>
    constexpr auto arrmc_apply<t_disambiguater>::apply(this t_self&& self, const t_fun& fun) -> auto {
        return base::template apply_private_fun<t_self, t_fun>(::lgo::f<t_self>(self), fun);
    }
}

#endif