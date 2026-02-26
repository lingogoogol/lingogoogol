#ifndef LGO_BASIC_SEQ_ARR_MC_BASE_VIRTUAL
#define LGO_BASIC_SEQ_ARR_MC_BASE_VIRTUAL

#include "../virtual/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename... t_type>
    class arrmc_virtual:
    public arrmc_tmerge<arrmc_virtual<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...>,
    public arrmc_tapply<arrmc_virtual<t_disambiguater, t_cvt, t_type...>, t_type...>,
    public arrmc_len<arrmc_virtual<t_disambiguater, t_cvt, t_type...>, t_type...>,
    public arrmc_type<arrmc_virtual<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...>,
    public arrmc_treduce<arrmc_virtual<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...>,
    public arrmc_tfilter<arrmc_virtual<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...>,
    public arrmc_tfind<arrmc_virtual<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {};
}

#endif