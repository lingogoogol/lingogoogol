#ifndef LGO_BASIC_SEQ_ARR_SUBD_DEF
#define LGO_BASIC_SEQ_ARR_SUBD_DEF

#include "decl.hpp"

#include "../main/def.hpp"
#include "../../main/subd/def.hpp"
#include "../../../type/lgo/.hpp"

namespace lgo {
    template<typename t_type>
    class arrsubd: public lgo, public seqsubd<t_type>, public arr {
    private:
        static_assert(is_arrd_cpt<rm_qcv<t_type>>);

        using base = seqsubd<t_type>;

        using base::seqsubd;

        template<typename t_type>
        friend class arrmd;
    public:
        using base::operator=;
        constexpr auto operator=(const arrsubd&) & -> arrsubd& = default;
    };
}

#endif