#ifndef LGO_BASIC_TYPE_CHA_BASIC_CVT
#define LGO_BASIC_TYPE_CHA_BASIC_CVT

#include "../int/p/basic.hpp"
#include "../constraint/.hpp"
#include "../tem/v/.hpp"

namespace lgo {
    template<is_basic_cha_cpt t_in>
    using basic_cha_to_basic_intp = tcond_basic<is_same_cpt<t_in, char8_t>, unsigned char
    , tcond_basic<is_same_cpt<t_in, char16_t>, std::uint_least16_t, std::uint_least32_t>>;
}

#endif