#ifndef LGO_BASIC_SEQ_ARR_MC_PACK_DEF
#define LGO_BASIC_SEQ_ARR_MC_PACK_DEF

#include "decl.hpp"
#include "cpt.hpp"
#include "base_def.hpp"

namespace lgo {
    template<typename t_type>
    struct pack_to_base_stu {};

    template<typename... t_type>
    struct pack_to_base_stu<pack<t_type...>> {
        using type = arrmc_base<t_type...>;
    };

    template<typename t_type>
    struct pack_from_base_stu {};

    template<typename... t_type>
    struct pack_from_base_stu<arrmc_base<t_type...>> {
        using type = pack<t_type...>;
    };

    struct pack_cvt {
        using is_kind_bind = is_pack_bind;
        using to_base = bind<tem<pack_to_base_stu>>;
        using from_base = bind<tem<pack_from_base_stu>>;
    };
}

LGO_EXPORT namespace lgo {
    template<typename... t_type>
    class pack: public lgo, public arrmc_pack<pack<t_type...>, pack_cvt, arrmc_data_copyable, t_type...> {
    private:
        using base = arrmc_pack<pack, pack_cvt, arrmc_data_copyable, t_type...>;
    public:
        using base::arrmc_pack;
    };

    template<typename... u_in>
    pack(const seq_data<u_in...>&) -> pack<u_in...>;
}

#endif