#ifndef LGO_BASIC_TYPE_CHA_BASIC_IMPL
#define LGO_BASIC_TYPE_CHA_BASIC_IMPL

#include "basic_decl.hpp"

#include "../../e/def.hpp"

namespace lgo {
    template<is_basic_intp_cpt t_out, is_basic_cha_cpt t_in>
    constexpr auto to_basic_intp(t_in in) -> t_out {
        if (in >= '0' && in <= '9') {
            return sc<t_out>(in - '0');
        }
        else if (in >= 'A' && in <= 'F') {
            return sc<t_out>(10 + in - 'A');
        }
        throw e_other{ U""_strmdc };
    }
    
    template<is_basic_cha_cpt t_out, is_basic_intp_cpt t_in>
    constexpr auto to_basic_cha(t_in in) -> t_out {
        if (in <= 9) {
            return sc<t_out>('0' + in);
        }
        else if (in <= 15) {
            return sc<t_out>('A' - 10 + in);
        }
        throw e_other{ U""_strmdc };
    }
}

#endif