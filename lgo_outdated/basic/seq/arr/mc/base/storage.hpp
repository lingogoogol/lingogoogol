#ifndef LGO_BASIC_SEQ_ARR_MC_BASE_STORAGE
#define LGO_BASIC_SEQ_ARR_MC_BASE_STORAGE

#include "../storage/.hpp"

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    class arrmc_storage
    : public arrmc_data<arrmc_storage<t_disambiguater, t_cvt, t_opt, t_type...>, t_cvt, t_opt, t_type...>
    , public arrmc_cmp<arrmc_storage<t_disambiguater, t_cvt, t_opt, t_type...>>
    , public arrmc_reduce<arrmc_storage<t_disambiguater, t_cvt, t_opt, t_type...>>
    , public arrmc_apply<arrmc_storage<t_disambiguater, t_cvt, t_opt, t_type...>>
    , public arrmc_replace<arrmc_storage<t_disambiguater, t_cvt, t_opt, t_type...>, t_cvt>
    , public arrmc_each<arrmc_storage<t_disambiguater, t_cvt, t_opt, t_type...>, t_cvt, t_type...>
    , public arrmc_merge<arrmc_storage<t_disambiguater, t_cvt, t_opt, t_type...>, t_cvt> {
    private:
        using base_data = arrmc_data<arrmc_storage, t_cvt, t_opt, t_type...>;
    public:
        using base_data::arrmc_data;
    };
}

#endif