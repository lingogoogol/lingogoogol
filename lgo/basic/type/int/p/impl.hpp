#ifndef LGO_BASIC_TYPE_INT_P_IMPL
#define LGO_BASIC_TYPE_INT_P_IMPL

#include "life.hpp"

#include "../n/life.hpp"
#include "../n/impl.hpp"
#include "../../fun/cast_impl.hpp"

namespace lgo {
	template<typename t_type>
    template<typename t_in>
    constexpr auto intp_class<t_type>::size(t_in in) -> intpc
    requires is_one_of_cpt<t_in, size_stack, size_content> {
        return base::size(in);
    }
    
    template<typename t_type>
    constexpr auto intp_class<t_type>::min() -> intp_class {
        return intp_class{ 0 };
    }

    template<typename t_type>
    constexpr auto intp_class<t_type>::max() -> intp_class {
        return --min();
    }

	template<typename t_type>
	constexpr auto intp_class<t_type>::basic() const -> typename base::type {
		return this->m_data;
	}

    template<typename t_type>
    constexpr auto intp_class<t_type>::sign() const -> boo {
        return U"1"_boo;
    }

    template<typename t_type>
    constexpr auto intp_class<t_type>::absp() const -> intp_class {
        return *this;
    }
    
    template<typename t_type>
    constexpr auto intp_class<t_type>::operator+() -> intp_class {
        return *this;
    }

    template<typename t_type>
    constexpr auto intp_class<t_type>::operator-() const -> intn_class<typename base::type> {
        intn_class<typename base::type> out{};
        out.m_data = this->m_data;
        return out;
    }

    template<typename t_type>
    template<typename t_self>
    constexpr auto intp_class<t_type>::operator++(this t_self&& self) -> t_self&& {
        ++self.m_data;
        return self.f<t_self>();
    }

    template<typename t_type>
    constexpr auto intp_class<t_type>::operator++(int) -> intp_class {
        intp_class out{ *this };
        ++*this;
        return out;
    }

    template<typename t_type>
    template<typename t_self>
    constexpr auto intp_class<t_type>::operator--(this t_self&& self) -> t_self&& {
        --self.m_data;
        return self.f<t_self>();
    }

    template<typename t_type>
    constexpr auto intp_class<t_type>::operator--(int) -> intp_class {
        intp_class out{ *this };
        --*this;
        return out;
    }
}

#endif