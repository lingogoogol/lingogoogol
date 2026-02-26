#ifndef LGO_BASIC_SEQ_ARR_MC_BASE_AUTO
#define LGO_BASIC_SEQ_ARR_MC_BASE_AUTO

#include "virtual.hpp"
#include "both.hpp"

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    class arrmc_auto: public arrmc_virtual<arrmc_auto<t_disambiguater, t_cvt, t_type...>, t_cvt, t_type...> {};

    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    requires (is_constructible_cpt<t_type> && ...)
    class arrmc_auto<t_disambiguater, t_cvt, t_opt, t_type...>
    : public arrmc_both<arrmc_auto<t_disambiguater, t_cvt, t_opt, t_type...>, t_cvt, t_opt, t_type...> {
    private:
        using base_both = arrmc_both<arrmc_auto, t_cvt, t_opt, t_type...>;
    public:
        using base_both::arrmc_both;
    };
}

#endif