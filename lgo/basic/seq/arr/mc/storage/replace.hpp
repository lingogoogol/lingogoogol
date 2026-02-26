#ifndef LGO_BASIC_SEQ_ARR_MC_STORAGE_REPLACE
#define LGO_BASIC_SEQ_ARR_MC_STORAGE_REPLACE

namespace lgo {
    template<typename t_disambiguater, typename t_cvt>
    class arrmc_replace_private: private arrmc_cvt_private<arrmc_replace_private<t_disambiguater, t_cvt>, t_cvt> {
    private:
        using base_cvt = arrmc_cvt_private<arrmc_replace_private, t_cvt>;

        template<is_bind_cpt t_bind, is_intp_v_cpt t_ind, is_intp_v_cpt t_replacement_ind
        , typename t_self, typename t_replacement, typename t_out>
        constexpr auto replace_impl(this t_self&& self, const t_replacement& replacement, t_out& out) -> void
        requires base_cvt::template is_kind_cpt_private<t_replacement> && base_cvt::template is_kind_cpt_private<t_out>;
    protected:
        template<is_bind_cpt t_bind, typename t_self, typename t_replacement>
        constexpr auto replace_private(this t_self&& self, const t_replacement& replacement) -> auto
        requires base_cvt::template is_kind_cpt_private<t_replacement>;
    };

    template<typename t_disambiguater, typename t_cvt>
    template<is_bind_cpt t_bind, is_intp_v_cpt t_ind, is_intp_v_cpt t_replacement_ind
    , typename t_self, typename t_replacement, typename t_out>
    constexpr auto arrmc_replace_private<t_disambiguater, t_cvt>
    ::replace_impl(this t_self&& self, const t_replacement& replacement, t_out& out) -> void
    requires base_cvt::template is_kind_cpt_private<t_replacement> && base_cvt::template is_kind_cpt_private<t_out> {
        constexpr static boo replace{ t_bind::template type<t_self::type<t_ind>>::val };
        if constexpr (replace.basic()) {
            out.ind<t_ind>() = replacement.ind<t_replacement_ind>();
        }
        else {
            out.ind<t_ind>() = ::lgo::f<t_self>(self).ind<t_ind>();
        }
        constexpr static intpc next_replacement_ind{ basic_condval
        (replace, t_replacement_ind::val + U"1"_intpc, t_replacement_ind::val) };
        constexpr static intpc next_ind{ t_ind::val + U"1"_intpc };
        if constexpr ((next_ind < t_self::len()).basic()) {
            ::lgo::f<t_self>(self).replace_impl<t_bind, v<next_ind>, v<next_replacement_ind>>(replacement, out);
        }
        return;
    }

    template<typename t_disambiguater, typename t_cvt>
    template<is_bind_cpt t_bind, typename t_self, typename t_replacement>
    constexpr auto arrmc_replace_private<t_disambiguater, t_cvt>
    ::replace_private(this t_self&& self, const t_replacement& replacement) -> auto
    requires base_cvt::template is_kind_cpt_private<t_replacement> {
        using out_type = t_self::template treplace<t_bind, t_replacement>;
        out_type out{};
        ::lgo::f<t_self>(self).replace_impl<t_bind, intpc::val_v<U"0">, intpc::val_v<U"0">>(replacement, out);
        return out;
    }

    template<typename t_disambiguater, typename t_cvt>
    class arrmc_replace: private arrmc_replace_private<arrmc_replace<t_disambiguater, t_cvt>, t_cvt> {
    private:
        using base = arrmc_replace_private<arrmc_replace, t_cvt>;
    public:
        template<is_bind_cpt t_bind, typename t_self, typename t_replacement>
        constexpr auto replace(this t_self&& self, const t_replacement& replacement) -> auto;
    };

    template<typename t_disambiguater, typename t_cvt>
    template<is_bind_cpt t_bind, typename t_self, typename t_replacement>
    constexpr auto arrmc_replace<t_disambiguater, t_cvt>
    ::replace(this t_self&& self, const t_replacement& replacement) -> auto {
        return self.base::replace_private<t_bind>(replacement);
    }
}

#endif