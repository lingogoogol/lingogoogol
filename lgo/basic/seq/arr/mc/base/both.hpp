#ifndef LGO_BASIC_SEQ_ARR_MC_BASE_BOTH
#define LGO_BASIC_SEQ_ARR_MC_BASE_BOTH

#include "virtual.hpp"
#include "storage.hpp"

namespace lgo {
    template<typename t_disambiguater, typename t_cvt, typename t_opt, typename... t_type>
    class arrmc_both: public arrmc_virtual<arrmc_both<t_disambiguater, t_cvt, t_opt, t_type...>, t_cvt, t_type...>
    , public arrmc_storage<arrmc_both<t_disambiguater, t_cvt, t_opt, t_type...>, t_cvt, t_opt, t_type...> {
    private:
        using base_storage = arrmc_storage<arrmc_both, t_cvt, t_opt, t_type...>;
    public:
        using base_storage::arrmc_storage;
    };
}

#endif