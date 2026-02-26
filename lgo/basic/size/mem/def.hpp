#ifndef LGO_BASIC_SIZE_MEM_DEF
#define LGO_BASIC_SIZE_MEM_DEF

#include "decl.hpp"
#include "cpt.hpp"

#include "../../seq/arr/mc/main/def.hpp"

namespace lgo {
    template<typename t_type>
    struct size_mem_to_base_stu {};

    template<typename... t_type>
    struct size_mem_to_base_stu<size_mem<t_type...>> {
        using type = arrmc_base<t_type...>;
    };

    template<typename t_type>
    struct size_mem_from_base_stu {};

    template<typename... t_type>
    struct size_mem_from_base_stu<arrmc_base<t_type...>> {
        using type = size_mem<t_type...>;
    };

    struct size_mem_cvt {
        using is_kind_bind = is_size_mem_bind;
        using to_base = bind<tem<size_mem_to_base_stu>>;
        using from_base = bind<tem<size_mem_from_base_stu>>;
    };
}

LGO_EXPORT namespace lgo {
    template<typename... t_type>
    class size_mem: public lgo, public arrmc_pack<size_mem<t_type...>, size_mem_cvt, arrmc_data_raw, t_type...> {
    private:
        using base = arrmc_pack<size_mem, size_mem_cvt, arrmc_data_raw, t_type...>;
    public:
        using base::arrmc_pack;
    };

    template<typename... t_type>
    size_mem(t_type&&...) -> size_mem<t_type...>;
}

#endif