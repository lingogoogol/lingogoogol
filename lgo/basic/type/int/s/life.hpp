#ifndef LGO_BASIC_TYPE_INT_S_LIFE
#define LGO_BASIC_TYPE_INT_S_LIFE

#include "def.hpp"

#include "../main/life.hpp"
#include "../../fun/cast_impl.hpp"

namespace lgo {
    template<is_basic_intp_cpt t_type>
    template<is_int_cpt t_in>
    constexpr ints_class<t_type>::ints_class(const t_in& in, void*): base{ in.m_data }, m_sign{ in.sign() } {}

    template<is_basic_intp_cpt t_type>
    constexpr ints_class<t_type>::ints_class(): base{}, m_sign{ U"1"_boo } {}

    template<is_basic_intp_cpt t_type>
    template<is_basic_intp_cpt t_in>
    constexpr ints_class<t_type>::ints_class(t_in in): base{ ::lgo::sc<typename base::type>(in) }, m_sign{ U"1"_boo } {}

    template<is_basic_intp_cpt t_type>
    template<is_basic_ints_cpt t_in>
    constexpr ints_class<t_type>::ints_class(t_in in)
    : base{ ::lgo::sc<typename base::type>(in >= 0 ? in : -in) }, m_sign{ in >= 0 } {}

    template<is_basic_intp_cpt t_type>
    template<is_int_cpt t_in>
    constexpr ints_class<t_type>::ints_class(const t_in& in): ints_class{ in, nullptr } {}

    template<is_basic_intp_cpt t_type>
    constexpr ints_class<t_type>::ints_class(const ints_class& in): ints_class{ in, nullptr } {}

    template<is_basic_intp_cpt t_type>
    template<is_int_cpt t_in>
    constexpr auto ints_class<t_type>::operator=(t_in in) & -> ints_class& {
        m_sign = in.sign();
        this->m_data = in.m_data;
        return *this;
    }
    
    template<is_basic_intp_cpt t_type>
    constexpr auto ints_class<t_type>::operator=(const ints_class& in) & -> ints_class& {
        return operator=<ints_class>(in);
    }
}

#endif