#ifndef LGO_BASIC_TYPE_INT_N_IMPL
#define LGO_BASIC_TYPE_INT_N_IMPL

#include "life.hpp"
#include "cpt.hpp"

#include "../p/life.hpp"
#include "../main/impl.hpp"
#include "../../fun/cast_impl.hpp"

namespace lgo {
    template<is_basic_intp_cpt t_type>
    template<typename t_in>
    constexpr auto intn_class<t_type>::size(t_in in) -> intpc
    requires is_one_of_cpt<t_in, size_stack, size_content> {
        return base::size(in);
    }
    
    template<is_basic_intp_cpt t_type>
    constexpr auto intn_class<t_type>::min() -> intn_class {
        return ++max();
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto intn_class<t_type>::max() -> intn_class {
        return intn_class{ 0 };
    }
    
    template<is_basic_intp_cpt t_type>
    constexpr auto intn_class<t_type>::basic() const -> typename base::type {
        return -this->m_data;
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto intn_class<t_type>::sign() const -> boo {
        return U"0"_boo;
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto intn_class<t_type>::absp() const -> intp_class<typename base::type> {
        return -(*this);
    }
    
    template<is_basic_intp_cpt t_type>
    constexpr auto intn_class<t_type>::operator+() const -> intn_class {
        return *this;
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto intn_class<t_type>::operator-() const -> intp_class<typename base::type> {
        intp_class<typename base::type> out{};
        out.m_data = this->m_data;
        return out;
    }

    template<is_basic_intp_cpt t_type>
    template<typename t_self>
    constexpr auto intn_class<t_type>::operator++(this t_self&& self) -> t_self&& {
        --self.m_data;
        return self.f<t_self>();
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto intn_class<t_type>::operator++(int) -> intn_class {
        intn_class out{ *this };
        ++*this;
        return out;
    }

    template<is_basic_intp_cpt t_type>
    template<typename t_self>
    constexpr auto intn_class<t_type>::operator--(this t_self&& self) -> t_self&& {
        ++self.m_data;
        return self.f<t_self>();
    }

    template<is_basic_intp_cpt t_type>
    constexpr auto intn_class<t_type>::operator--(int) -> intn_class {
        intn_class out{ *this };
        --*this;
        return out;
    }
}

#endif