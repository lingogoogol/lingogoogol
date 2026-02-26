#ifndef LGO_BASIC_SEQ_ARR_MC_STORAGE_MERGE
#define LGO_BASIC_SEQ_ARR_MC_STORAGE_MERGE

namespace lgo {
    template<typename t_disambiguater, typename t_cvt>
    class arrmc_merge_private: private arrmc_cvt_private<arrmc_merge_private<t_disambiguater, t_cvt>, t_cvt> {
    private:
        using base_cvt = arrmc_cvt_private<arrmc_merge_private, t_cvt>;
        template<typename u_in>
        constexpr static cpt is_kind_cpt{ arrmc_cvt_private<arrmc_merge_private
        <t_disambiguater, t_cvt>, t_cvt>::template is_kind_cpt_private<u_in> };

        template<is_intp_v_cpt t_ind, is_intp_v_cpt t_in_ind, typename t_out, typename t_in, typename... t_other>
        constexpr static auto merge_impl(t_out& out, const t_in& in, const t_other&... other) -> void
        requires is_kind_cpt<t_out> && is_kind_cpt<t_in> && (is_kind_cpt<t_other> && ...);
    protected:
        template<typename t_self, typename... t_in>
        constexpr auto merge_private(this const t_self& self, const t_in&... in) -> auto;
        template<typename t_self, typename... t_in>
        constexpr static fun merge_private_fun{ &merge_private<t_self, t_in...> };
    };
    
    template<typename t_disambiguater, typename t_cvt>
    template<is_intp_v_cpt t_ind, is_intp_v_cpt t_in_ind, typename t_out, typename t_in, typename... t_other>
    constexpr auto arrmc_merge_private<t_disambiguater, t_cvt>::merge_impl(t_out& out, const t_in& in, const t_other&... other) -> void
    requires is_kind_cpt<t_out> && is_kind_cpt<t_in> && (is_kind_cpt<t_other> && ...) {
        out.ind_internal<t_ind>() = in.ind_internal<t_in_ind>();
        constexpr static intpc next_ind{ t_ind::val + U"1"_intpc };
        constexpr static intpc next_in_ind{ t_in_ind::val + U"1"_intpc };
        if constexpr ((next_ind >= t_out::len()).basic()) {}
        else if constexpr ((next_in_ind >= t_in::len()).basic()) {
            merge_impl<v<next_ind>, intpc::val_v<U"0">>(out, other...);
        }
        else {
            merge_impl<v<next_ind>, v<next_in_ind>>(out, in, other...);
        }
        return;
    }
    
    template<typename t_disambiguater, typename t_cvt>
    template<typename t_self, typename... t_in>
    constexpr auto arrmc_merge_private<t_disambiguater, t_cvt>
    ::merge_private(this const t_self& self, const t_in&... in) -> auto {
        using out_type = t_self::template tmerge<t_in...>;
        out_type out{};
        if constexpr ((out_type::len() != U"0"_intpc).basic()) {
            merge_impl<intpc::val_v<U"0">, intpc::val_v<U"0">>(out, self, in...);
        }
        return out;
    }

    template<typename t_disambiguater, typename t_cvt>
    class arrmc_merge: private arrmc_merge_private<arrmc_merge<t_disambiguater, t_cvt>, t_cvt> {
    private:
        using base = arrmc_merge_private<arrmc_merge<t_disambiguater, t_cvt>, t_cvt>;
    public:
        template<typename t_self, typename... t_in>
        constexpr auto merge(this const t_self& self, const t_in&... in) -> auto;
    };

    template<typename t_disambiguater, typename t_cvt>
    template<typename t_self, typename... t_in>
    constexpr auto arrmc_merge<t_disambiguater, t_cvt>::merge(this const t_self& self, const t_in&... in) -> auto {
        return base::template merge_private_fun<t_self, t_in...>(self, in...);
    }
}

#endif