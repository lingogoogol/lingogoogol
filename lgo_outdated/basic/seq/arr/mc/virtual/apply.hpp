#ifndef LGO_BASIC_SEQ_ARR_MC_VIRTUAL_APPLY
#define LGO_BASIC_SEQ_ARR_MC_VIRTUAL_APPLY

namespace lgo {
    template<typename t_disambiguater, typename... t_type>
    class arrmc_tapply_private {
    protected:
        template<is_bind_cpt u_bind>
        using tapply_private = u_bind::template type<t_type...>;
    };

    template<typename t_disambiguater, typename... t_type>
    class arrmc_tapply: private arrmc_tapply_private<arrmc_tapply<t_disambiguater, t_type...>, t_type...> {
    private:
        using base = arrmc_tapply_private<arrmc_tapply, t_type...>;
    public:
        template<is_bind_cpt u_bind>
        using tapply = base::template tapply_private<u_bind>;
    };
}

#endif