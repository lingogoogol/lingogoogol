#ifndef LGO_BASIC_SEQ_MEM_SUBD_DEF
#define LGO_BASIC_SEQ_MEM_SUBD_DEF

#include "decl.hpp"

#include "../../main/subd/def.hpp"

namespace lgo {
    template<typename t_type>
    class memsubd: public lgo, public seqsubd<t_type>, public seqiod {
    private:
        static_assert(is_memd_cpt<rm_qcv<t_type>>);

        using base = seqsubd<t_type>;

        using base::seqsubd;

        friend class memmd;
    public:
        using base::operator=;
        constexpr auto operator=(const memsubd&) & -> memsubd& = default;
    };
}

#endif