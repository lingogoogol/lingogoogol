#ifndef LGO_BASIC_SEQ_ARR_MC_VIRTUAL_CVT
#define LGO_BASIC_SEQ_ARR_MC_VIRTUAL_CVT

namespace lgo {
    template<typename t_disambiguater, typename t_cvt>
    class arrmc_cvt_private {
    protected:
        template<typename t_in>
        constexpr static cpt is_kind_cpt_private{ t_cvt::is_kind_bind::template type<t_in>::val.basic() };
        template<typename t_in>
        using to_base_private = t_cvt::to_base::template type<t_in>;
        template<typename t_in>
        using from_base_private = t_cvt::from_base::template type<t_in>;
    };
}

#endif