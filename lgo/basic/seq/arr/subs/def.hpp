#ifndef LGO_BASIC_SEQ_ARR_SUBS_DEF
#define LGO_BASIC_SEQ_ARR_SUBS_DEF

#include "decl.hpp"

#include "../cpt/.hpp"
#include "../mc/.hpp"
#include "../main/def.hpp"
#include "../../main/subs/def.hpp"
#include "../../../type/deduce/cond.hpp"
#include "../../../type/tem/v/.hpp"
#include "../../../type/fun/cast_impl.hpp"
#include "../../../type/boo/.hpp"
#include "../../../type/int/p/def.hpp"
#include "../../../type/lgo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_type>
    class arrsubs: public lgo, public seqsubs<t_type>, public arr {
    private:
        static_assert(is_arrs_cpt<rm_qcv<t_type>>);

        using base = seqsubs<t_type>;

        using base::seqsubs;

        template<typename t_type, is_intp_v_cpt t_len>
        friend class arrms;
        template<typename t_type>
        friend class arrmd;
    public:
        using base::operator=;
        constexpr auto operator=(const arrsubs&) & -> arrsubs& = default;
    };
}

#endif