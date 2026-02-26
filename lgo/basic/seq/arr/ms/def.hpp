#ifndef LGO_BASIC_SEQ_ARR_MS_DEF
#define LGO_BASIC_SEQ_ARR_MS_DEF

#include "decl.hpp"

#include "../main/def.hpp"
#include "../../main/ms/empty.hpp"
#include "../../main/ms/nonempty_def.hpp"
#include "../../../type/lgo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_type, is_intp_v_cpt t_len>
    class arrms: public lgo, public seqms<t_type, t_len>, public arr {
    private:
        using base = seqms<t_type, t_len>;
    public:
        using base::size;
        using arr::size;

        using base::operator=;
        constexpr auto operator=(const arrms& in) & -> arrms&;
    };
}

#endif