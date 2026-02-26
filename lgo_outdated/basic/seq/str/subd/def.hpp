#ifndef LGO_BASIC_SEQ_STR_SUBD_DEF
#define LGO_BASIC_SEQ_STR_SUBD_DEF

#include "decl.hpp"

#include "../main/def.hpp"
#include "../../main/subd/def.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_type>
    class strsubd: public lgo, public seqsubd<t_type>, public str, public seqiod {
    private:
        static_assert(is_strd_cpt<rm_qcv<t_type>>);

        using base = seqsubd<t_type>;

        using base::seqsubd;

        template<is_cha_cpt t_type>
        friend class strmd;
    public:
        using base::operator=;
        constexpr auto operator=(const strsubd&) & -> strsubd& = default;
    };
}

#endif