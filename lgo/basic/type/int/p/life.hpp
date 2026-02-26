#ifndef LGO_BASIC_TYPE_INT_P_LIFE
#define LGO_BASIC_TYPE_INT_P_LIFE

#include "def.hpp"

#include "../../fun/cast_impl.hpp"

namespace lgo {
	template<typename t_type>
	template<is_basic_int_cpt t_in>
	constexpr intp_class<t_type>::intp_class(t_in in): base{ ::lgo::sc<typename base::type>(in) } {}

    template<typename t_type>
    template<is_int_cpt t_in>
    constexpr intp_class<t_type>::intp_class(const t_in& in): base{ ::lgo::sc<typename base::type>(in.m_data) } {
        if constexpr (is_ints_cpt<t_in>) {
            if ((!in.sign() && boo{ in.m_data != 0 }).basic()) {
                throw e_other{ U""_strmdc };
            }
        }
    }

    template<typename t_type>
    constexpr intp_class<t_type>::intp_class(const intp_class& in): base{ in.m_data } {}

    template<typename t_type>
    template<is_int_cpt t_in>
    constexpr auto intp_class<t_type>::operator=(t_in in) & -> intp_class& {
        this->m_data = intp_class{ in }.base::m_data;
        return *this;
    }
    
    template<typename t_type>
    constexpr auto intp_class<t_type>::operator=(const intp_class& in) & -> intp_class& {
        return operator=<intp_class>(in);
    }
}

#endif