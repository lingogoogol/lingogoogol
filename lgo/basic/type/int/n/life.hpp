#ifndef LGO_BASIC_TYPE_INT_N_LIFE
#define LGO_BASIC_TYPE_INT_N_LIFE

#include "def.hpp"

namespace lgo {
    template<is_basic_intp_cpt t_type>
    template<is_basic_intp_cpt t_in>
    constexpr intn_class<t_type>::intn_class(t_in in): base{ ::lgo::sc<typename base::type>(in) } {}

    template<is_basic_intp_cpt t_type>
    template<is_basic_ints_cpt t_in>
    constexpr intn_class<t_type>::intn_class(t_in in): base{ ::lgo::sc<typename base::type>(-in) } {}

    template<is_basic_intp_cpt t_type>
    template<is_int_cpt t_in>
    constexpr intn_class<t_type>::intn_class(const t_in& in): base{ ::lgo::sc<typename base::type>(in.m_data) } {}

    template<is_basic_intp_cpt t_type>
    constexpr intn_class<t_type>::intn_class(const intn_class& in): base{ in.m_data } {}

    template<is_basic_intp_cpt t_type>
    template<is_int_cpt t_in>
    constexpr auto intn_class<t_type>::operator=(t_in in) & -> intn_class& {
        this->m_data = intn_class{ in }.base::m_data;
        return *this;
    }
    
    template<is_basic_intp_cpt t_type>
    constexpr auto intn_class<t_type>::operator=(const intn_class& in) & -> intn_class& {
        return operator=<intn_class>(in);
    }
}

#endif