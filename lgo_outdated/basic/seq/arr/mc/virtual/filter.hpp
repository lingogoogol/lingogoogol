#ifndef LGO_BASIC_SEQ_ARR_MC_VIRTUAL_FILTER
#define LGO_BASIC_SEQ_ARR_MC_VIRTUAL_FILTER

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_tfilter_private: private arrmc_cvt_private<arrmc_tfilter_private<t_disambiguater, t_cvt, t_type...>, t_cvt>
    , private arrmc_treduce_private<arrmc_tfilter_private<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base_cvt = arrmc_cvt_private<arrmc_tfilter_private, t_cvt>;
        using base_treduce = arrmc_treduce_private<arrmc_tfilter_private, t_cvt, t_type...>;

        template<is_bind_cpt u_tem>
        struct tfilter_stu {
            template<typename v_previous, typename v_current, is_intp_v_cpt v_pos, typename>
            struct bind_stu {
                struct type {
                    using processed = tcond_basic<u_tem::template type<v_current>::val.basic(), typename v_previous
                    ::template tmerge<typename base_cvt::template from_base_private<arrmc_base<v_current>>>, v_previous>;
                    using state = void;
                };
            };
        };
    protected:
        template<is_bind_cpt u_bind>
        using tfilter_private = base_treduce::template treduce_private<bind<tem<tfilter_stu<u_bind>::template bind_stu>>
        , typename base_cvt::template from_base_private<arrmc_base<>>, void>;
        
        template<is_bind_cpt u_tem>
        constexpr static boo tcontain_private{ tfilter_private<u_tem>::len() != U"0"_intpc };
    };

    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_tfilter: private arrmc_tfilter_private<arrmc_tfilter<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {
    private:
        using base = arrmc_tfilter_private<arrmc_tfilter, t_cvt, t_type...>;
    public:
        template<is_bind_cpt u_bind>
        using tfilter = base::template tfilter_private<u_bind>;
        
        template<is_bind_cpt u_tem>
        constexpr static boo tcontain
        { arrmc_tfilter_private<arrmc_tfilter, t_cvt, t_type...>::template tcontain_private<u_tem> };
    };
}

#endif