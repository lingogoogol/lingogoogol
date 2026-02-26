#ifndef LGO_BASIC_SEQ_ARR_MAIN_OPT_FOR_ALL_DEF
#define LGO_BASIC_SEQ_ARR_MAIN_OPT_FOR_ALL_DEF

#include "cpt.hpp"
#include "decl.hpp"

#include "../../../mc/pack/base_def.hpp"

namespace lgo {
    template<typename t_type>
    struct arr_for_all_to_base_stu {};

    template<typename... t_type>
    struct arr_for_all_to_base_stu<arr_for_all<t_type...>> {
        using type = arrmc_base<t_type...>;
    };

    template<typename t_type>
    struct arr_for_all_from_base_stu {};

    template<typename... t_type>
    struct arr_for_all_from_base_stu<arrmc_base<t_type...>> {
        using type = arr_for_all<t_type...>;
    };

    struct arr_for_all_cvt {
        using is_kind_bind = is_arr_for_all_bind;
        using to_base = bind<tem<arr_for_all_to_base_stu>>;
        using from_base = bind<tem<arr_for_all_from_base_stu>>;
    };
}

LGO_EXPORT namespace lgo {
    template<typename... t_type>
    class arr_for_all: public lgo, public arrmc_pack<arr_for_all<t_type...>, arr_for_all_cvt, arrmc_data_raw, t_type...> {
    private:
        using base = arrmc_pack<arr_for_all, arr_for_all_cvt, arrmc_data_raw, t_type...>;
    public:
        using base::arrmc_pack;
    };

    template<typename... t_type>
    arr_for_all(t_type&&...) -> arr_for_all<t_type...>;
}

#endif