#ifndef LGO_BASIC_TYPE_INT_S_BASIC_CVT
#define LGO_BASIC_TYPE_INT_S_BASIC_CVT

#include "../p/basic.hpp"
#include "../../constraint/.hpp"
#include "../../tem/v/.hpp"

namespace lgo {
    template<is_basic_ints_cpt t_in>
    using basic_ints_to_basic_intp =
    tcond_basic<is_same_cpt<t_in, char>, unsigned char, tcond_basic<is_same_cpt<t_in, short>, unsigned short
    , tcond_basic<is_same_cpt<t_in, int>, unsigned, tcond_basic<is_same_cpt<t_in, long>, unsigned long, unsigned long long>>>>;
    
    template<is_basic_intp_cpt t_in>
    using basic_intp_to_basic_ints = 
    tcond_basic<is_same_cpt<t_in, unsigned char>, char, tcond_basic<is_same_cpt<t_in, unsigned short>, short
    , tcond_basic<is_same_cpt<t_in, unsigned>, int, tcond_basic<is_same_cpt<t_in, unsigned long>, long, long long>>>>;
}

#endif