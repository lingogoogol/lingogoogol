#ifndef LGO_BASIC_SEQ_ARR_MC_STORAGE_CMP
#define LGO_BASIC_SEQ_ARR_MC_STORAGE_CMP

namespace lgo {
    template<typename t_disambiguater>
    class arrmc_cmp_private {
    protected:
        template<typename t_self>
        constexpr auto eq_private(this const t_self& self, const t_self& in) -> boo;
        template<typename t_self>
        constexpr auto neq_private(this const t_self& self, const t_self& in) -> boo;
    };

    template<typename t_disambiguater>
    template<typename t_self>
    constexpr auto arrmc_cmp_private<t_disambiguater>::eq_private(this const t_self& self, const t_self& in) -> boo {
        if constexpr ((t_self::len() > U"0"_intpc).basic()) {
            return self.m_data == in.m_data && self.m_next == in.m_next;
        }
        else {
            return U"1"_boo;
        }
    }

    template<typename t_disambiguater>
    template<typename t_self>
    constexpr auto arrmc_cmp_private<t_disambiguater>::neq_private(this const t_self& self, const t_self& in) -> boo {
        if constexpr ((t_self::len() > U"0"_intpc).basic()) {
            return self.m_data != in.m_data || self.m_next != in.m_next;
        }
        else {
            return U"0"_boo;
        }
    }

    template<typename t_disambiguater>
    class arrmc_cmp: private arrmc_cmp_private<arrmc_cmp<t_disambiguater>> {
    private:
        using base = arrmc_cmp_private<arrmc_cmp>;
    public:
        template<typename t_self>
        constexpr auto operator==(this const t_self& self, const t_self& in) -> boo;
        template<typename t_self>
        constexpr auto operator!=(this const t_self& self, const t_self& in) -> boo;
    };

    template<typename t_disambiguater>
    template<typename t_self>
    constexpr auto arrmc_cmp<t_disambiguater>::operator==(this const t_self& self, const t_self& in) -> boo {
        return self.base::eq_private(in);
    }

    template<typename t_disambiguater>
    template<typename t_self>
    constexpr auto arrmc_cmp<t_disambiguater>::operator!=(this const t_self& self, const t_self& in) -> boo {
        return self.base::neq_private(in);
    }
}

#endif