#ifndef LGO_BASIC_SEQ_ARR_MC_STORAGE_REDUCE
#define LGO_BASIC_SEQ_ARR_MC_STORAGE_REDUCE

namespace lgo {
    template<typename t_disambiguater>
    class arrmc_reduce_private {
    private:
        template<typename t_self, is_fun_cpt t_fun, typename t_start, typename t_state, typename... t_other>
        constexpr auto reduce_impl(this const t_self& self, const t_fun& fun
        , t_start&& start, t_state&& state, intpc ind, t_other&&... other) -> auto;
    protected:
        template<typename t_self, is_fun_cpt t_fun, typename t_start, typename t_state, typename... t_other>
        constexpr auto reduce_private(this const t_self& self, const t_fun& fun
        , t_start&& start, t_state&& state, t_other&&... other) -> auto;
    };

    template<typename t_disambiguater>
    template<typename t_self, is_fun_cpt t_fun, typename t_previous, typename t_state, typename... t_other>
    constexpr auto arrmc_reduce_private<t_disambiguater>::reduce_impl(this const t_self& self, const t_fun& fun
    , t_previous&& previous, t_state&& state, intpc ind, t_other&&... other) -> auto {
        auto stu{ fun(previous, ind, state, self.m_data, ::lgo::f<t_other>(other.m_data)...) };
        return self.m_next.reduce_impl(fun, ::lgo::r(stu.processed)
        , ::lgo::r(stu.state), ind + U"1"_intpc, ::lgo::f<t_other>(other)...);
    }

    template<typename t_disambiguater>
    template<typename t_self, is_fun_cpt t_fun, typename t_start, typename t_state, typename... t_other>
    constexpr auto arrmc_reduce_private<t_disambiguater>::reduce_private(this const t_self& self, const t_fun& fun
    , t_start&& start, t_state&& state, t_other&&... other) -> auto {
        return self.reduce_impl(fun, ::lgo::f<t_start>(start)
        , ::lgo::f<t_state>(state), U"0"_intpc, ::lgo::f<t_other>(other)...).processed;
    }

    template<typename t_disambiguater>
    class arrmc_reduce: private arrmc_reduce_private<arrmc_reduce<t_disambiguater>> {
    private:
        using base = arrmc_reduce_private<arrmc_reduce>;
    public:
        template<typename t_self, is_fun_cpt t_fun, typename t_start, typename t_state, typename... t_other>
        constexpr auto reduce(this const t_self& self, const t_fun& fun
        , t_start&& start, t_state&& state, t_other&&... other) -> auto;
    };

    template<typename t_disambiguater>
    template<typename t_self, is_fun_cpt t_fun, typename t_start, typename t_state, typename... t_other>
    constexpr auto arrmc_reduce<t_disambiguater>::reduce(this const t_self& self, const t_fun& fun
    , t_start&& start, t_state&& state, t_other&&... other) -> auto {
        return self.base::reduce_private(fun
        , ::lgo::f<t_start>(start), ::lgo::f<t_state>(state), ::lgo::f<t_other>(other)...);
    }
}

#endif