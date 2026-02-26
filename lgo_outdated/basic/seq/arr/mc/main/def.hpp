#ifndef LGO_BASIC_SEQ_ARR_MC_MAIN_DEF
#define LGO_BASIC_SEQ_ARR_MC_MAIN_DEF

#include "cpt.hpp"

#include "../base/base_decl.hpp"
#include "../base/auto.hpp"

namespace lgo {
    template<typename t_type>
    struct arrmc_to_base_stu {};

    template<typename... t_type>
    struct arrmc_to_base_stu<arrmc<t_type...>> {
        using type = arrmc_base<t_type...>;
    };

    template<typename t_type>
    struct arrmc_from_base_stu {};

    template<typename... t_type>
    struct arrmc_from_base_stu<arrmc_base<t_type...>> {
        using type = arrmc<t_type...>;
    };

    struct arrmc_main_cvt {
        using is_kind_bind = is_arrmc_bind;
        using to_base = bind<tem<arrmc_to_base_stu>>;
        using from_base = bind<tem<arrmc_from_base_stu>>;
    };
}

LGO_EXPORT namespace lgo {
    template<typename... t_type>
    class arrmc: public lgo, public arrmc_auto<arrmc<t_type...>, arrmc_main_cvt, arrmc_data_copyable, t_type...> {
    private:
        using base = arrmc_auto<arrmc, arrmc_main_cvt, arrmc_data_copyable, t_type...>;
    public:
        using base::arrmc_auto;
    };

    template<typename... t_type>
    arrmc(t_type...) -> arrmc<t_type...>;
}

#endif