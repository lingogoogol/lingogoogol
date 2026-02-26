#ifndef LGO_BASIC_SEQ_MAIN_MAIN_OPT_DATA_DEF
#define LGO_BASIC_SEQ_MAIN_MAIN_OPT_DATA_DEF

#include "decl.hpp"

#include "../../../../arr/mc/main/def.hpp"
#include "../../../../arr/mc/pack/.hpp"
#include "../../../../../type/lgo/.hpp"

namespace lgo {
    template<typename t_type>
    struct seq_data_to_base_stu {};

    template<typename... t_type>
    struct seq_data_to_base_stu<seq_data<t_type...>> {
        using type = arrmc_base<t_type...>;
    };

    template<typename t_type>
    struct seq_data_from_base_stu {};

    template<typename... t_type>
    struct seq_data_from_base_stu<arrmc_base<t_type...>> {
        using type = seq_data<t_type...>;
    };

    struct seq_data_cvt {
        using is_kind_bind = is_seq_data_bind;
        using to_base = bind<tem<seq_data_to_base_stu>>;
        using from_base = bind<tem<seq_data_from_base_stu>>;
    };
}

LGO_EXPORT namespace lgo {
    template<typename... t_type>
    class seq_data: public lgo, public arrmc_pack<seq_data<t_type...>, seq_data_cvt, arrmc_data_raw, t_type...> {
    private:
        using base = arrmc_pack<seq_data, seq_data_cvt, arrmc_data_raw, t_type...>;
    public:
        using base::arrmc_pack;
        constexpr seq_data(const seq_data&) requires false;
    };

    template<typename... u_in>
    seq_data(u_in&&...) -> seq_data<u_in...>;
}

#endif