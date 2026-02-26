#ifndef LGO_BASIC_SEQ_STR_MS_ALIAS_DECL
#define LGO_BASIC_SEQ_STR_MS_ALIAS_DECL

#include "../../../type/int/p/cpt.hpp"
#include "../../../type/tem/t/def.hpp"

namespace lgo {
    template<is_intp_v_cpt t_len>
    class strms8;
    template<is_intp_v_cpt t_len>
    class strms16;
    template<is_intp_v_cpt t_len>
    class strms32;
    template<is_intp_v_cpt t_len>
    class strmsc;
}

LGO_EXPORT namespace lgo {
    template<is_intp_v_cpt t_len>
    using strms8_t = t<strms8<t_len>>;
    template<is_intp_v_cpt t_len>
    using strms16_t = t<strms16<t_len>>;
    template<is_intp_v_cpt t_len>
    using strms32_t = t<strms32<t_len>>;
    template<is_intp_v_cpt t_len>
    using strmsc_t = t<strmsc<t_len>>;
}

#endif