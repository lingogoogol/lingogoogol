#ifndef LGO_BASIC_SEQ_ARR_MC_VIRTUAL_MERGE
#define LGO_BASIC_SEQ_ARR_MC_VIRTUAL_MERGE

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_tmerge_private
    : private arrmc_cvt_private<arrmc_tmerge_private<t_disambiguater, t_cvt, t_type...>, t_cvt> {
    private:
        using base_cvt = arrmc_cvt_private<arrmc_tmerge_private, t_cvt>;
        template<typename u_in>
        constexpr static cpt is_kind_cpt{ arrmc_cvt_private<arrmc_tmerge_private
        <t_disambiguater, t_cvt, t_type...>, t_cvt>::template is_kind_cpt_private<u_in> };

        template<typename...>
        struct tmerge_stu {};

        template<typename u_in>
        requires is_arrmc_base_cpt<u_in>
        struct tmerge_stu<u_in> {
            using type = u_in;
        };

        template<typename... u_type1, typename... u_type2, typename... u_other>
        requires (is_arrmc_base_cpt<u_other> && ...)
        struct tmerge_stu<arrmc_base<u_type1...>, arrmc_base<u_type2...>, u_other...> {
            using type = tmerge_stu<arrmc_base<u_type1..., u_type2...>, u_other...>::type;
        };
    protected:
        template<typename... u_in>
        requires (is_kind_cpt<u_in> && ...)
        using tmerge_private = base_cvt::template from_base_private<typename tmerge_stu
        <arrmc_base<t_type...>, typename base_cvt::template to_base_private<u_in>...>::type>;
    };

    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_tmerge: private arrmc_tmerge_private<arrmc_tmerge<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base = arrmc_tmerge_private<arrmc_tmerge, t_cvt, t_type...>;
    public:
        template<typename... u_in>
        using tmerge = base::template tmerge_private<u_in...>;
    };
}

#endif