#ifndef LGO_BASIC_TYPE_BOO_IMPL
#define LGO_BASIC_TYPE_BOO_IMPL

#include "def.hpp"

#include "../fun/cast_impl.hpp"

namespace lgo {
    template<basic_literal_str t_in>
    constexpr auto boo::make() -> boo {
        return boo{ t_in.m_data[0] == U'1' };
    }

    template<typename t_in>
    constexpr auto boo::size(const t_in&) -> intpc
    requires is_one_of_cpt<t_in, size_stack, size_content, size_mem<>> {
        return intpc{ sizeof(bool) };
    }

    constexpr boo::boo(): m_data{ false } {}

    constexpr boo::boo(bool in): m_data{ in } {}

    constexpr auto boo::basic() const -> bool {
        return m_data;
    }

    constexpr auto boo::cpt() const -> ::lgo::cpt {
        return basic();
    }

    constexpr auto boo::operator==(boo in) const -> boo {
        return boo{ m_data == in.m_data };
    }

    constexpr auto boo::operator!=(boo in) const -> boo {
        return !(*this == in);
    }

    constexpr auto boo::operator!() const -> boo {
        return boo{ !m_data };
    }

    constexpr auto boo::operator&&(boo in) const -> boo {
        return boo{ m_data && in.m_data };
    }

    constexpr auto boo::operator||(boo in) const -> boo {
        return boo{ m_data || in.m_data };
    }
}

#endif