#ifndef LGO_BASIC_TYPE_INT_S_IMPL
#define LGO_BASIC_TYPE_INT_S_IMPL

#include "life.hpp"
#include "basic_cvt.hpp"

#include "../main/impl.hpp"
#include "../../boo/literal.hpp"

namespace lgo {
    template<is_basic_intp_cpt t_type>
    template<typename t_in>
    constexpr auto ints_class<t_type>::size(const t_in& in) -> intpc
    requires is_one_of_cpt<t_in, size_stack, size_mem<>> {
        return base::size(in) + boo::size(in);
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto ints_class<t_type>::size(size_content) -> intpc {
        return base::size(size_content{});
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto ints_class<t_type>::basic() const-> basic_intp_to_basic_ints<typename base::type> {
        using out_type = basic_intp_to_basic_ints<typename base::type>;
        out_type out{ ::lgo::sc<out_type>(this->m_data) };
        if constexpr (is_basic_intp_cpt<out_type>) {
            return out;
        }
        else {
            return m_sign.basic() ? out : -out;
        }
    }
    
    template<is_basic_intp_cpt t_type>
    constexpr auto ints_class<t_type>::sign() const -> boo {
        return m_sign;
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto ints_class<t_type>::absp() const -> intp_class<typename base::type> {
        return intp_class<typename base::type>{ this->m_data };
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto ints_class<t_type>::operator+() const -> ints_class {
        return *this;
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto ints_class<t_type>::operator-() const -> ints_class {
        ints_class out{ *this };
        out.m_sign = !out.m_sign;
        return out;
    }
    
    template<is_basic_intp_cpt t_type>
    template<typename t_self>
    constexpr auto ints_class<t_type>::operator++(this t_self&& self) -> t_self&& {
        if (self.m_data == 0) {
            self.m_sign = U"1"_boo;
            ++self.m_data;
        }
        else if (self.m_sign.basic()) {
            ++self.m_data;
        }
        else {
            --self.m_data;
        }
        return self.f<t_self>();
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto ints_class<t_type>::operator++(int) -> ints_class {
        ints_class out{ *this };
        ++*this;
        return out;
    }

    template<is_basic_intp_cpt t_type>
    template<typename t_self>
    constexpr auto ints_class<t_type>::operator--(this t_self&& self) -> t_self&& {
        if (self.m_data == 0) {
            self.m_sign = U"0"_boo;
            ++self.m_data;
        }
        else if (self.m_sign.basic()) {
            --self.m_data;
        }
        else {
            ++self.m_data;
        }
        return self.f<t_self>();
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto ints_class<t_type>::operator--(int) -> ints_class {
        ints_class out{ *this };
        --*this;
        return out;
    }
}

#endif