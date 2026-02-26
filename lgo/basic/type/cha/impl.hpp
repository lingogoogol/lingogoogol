#ifndef LGO_BASIC_TYPE_CHA_IMPL
#define LGO_BASIC_TYPE_CHA_IMPL

#include "def.hpp"

#include "../int/p/life.hpp"
#include "../fun/cast_impl.hpp"

namespace lgo {
    template<is_basic_cha_cpt t_type>
    template<basic_literal_str t_in>
    constexpr auto cha<t_type>::make() -> cha {
        return cha{ ::lgo::sc<type>(t_in.m_data[0]) };
    }
    
    template<is_basic_cha_cpt t_type>
    template<typename t_in>
    constexpr auto cha<t_type>::size(const t_in&) -> intpc
    requires is_one_of_cpt<t_in, size_stack, size_content, size_mem<>> {
        return intpc{ sizeof(type) };
    }
    
    template<is_basic_cha_cpt t_type>
    template<is_cha_cpt, is_str_cpt u_in>
    constexpr auto cha<t_type>::str_get(const u_in& in) -> cha {
        if ((in.len() < U"1"_intpc).basic()) {
            throw get_not_enough{ U"1"_intpc };
        }

        in.getpos(U"1"_intpc);
        return in[U"0"_intpc];
    }

    template<is_basic_cha_cpt t_type>
    template<is_str_cpt u_out>
    constexpr auto cha<t_type>::str_pad(cha in, u_out& out) -> void {
        if ((out.len() < U"1"_intpc).basic()) {
            throw pad_not_enough{ U"1"_intpc };
        }

        out[U"0"_intpc] = in;
        out.padpos(U"1"_intpc);
        return;
    }
    
    template<is_basic_cha_cpt t_type>
    template<is_cha_cpt, is_mem_cpt u_in>
    constexpr auto cha<t_type>::mem_get(const u_in& in) -> cha {
        return cha{ ::lgo::sc<type>(in.get<intp_class<basic_cha_to_basic_intp<type>>>().basic()) };
    }
    
    template<is_basic_cha_cpt t_type>
    template<is_mem_cpt u_out>
    constexpr auto cha<t_type>::mem_pad(cha in, u_out& out) -> void {
        out.pad(intp_class<basic_cha_to_basic_intp<type>>{ ::lgo::sc<basic_cha_to_basic_intp<type>>(in.m_data) });
        return;
    }

    template<is_basic_cha_cpt t_type>
    constexpr cha<t_type>::cha(): m_data{ '\0' } {}
    
    template<is_basic_cha_cpt t_type>
    constexpr cha<t_type>::cha(type in): m_data{ in } {}

    template<is_basic_cha_cpt t_type>
    template<is_cha_cpt t_in>
    constexpr cha<t_type>::cha(t_in in): m_data{ ::lgo::sc<type>(in.m_data) } {}

    template<is_basic_cha_cpt t_type>
    constexpr auto cha<t_type>::basic() const -> type {
        return m_data;
    }

    template<is_basic_cha_cpt t_type>
    template<is_int_cpt t_in>
    constexpr auto cha<t_type>::operator+(t_in in) const -> cha {
        return cha{ ::lgo::sc<type>(m_data + in.basic()) };
    }

    template<is_basic_cha_cpt t_type>
    template<is_int_cpt t_in>
    constexpr auto cha<t_type>::operator-(t_in in) const -> cha {
        return cha{ ::lgo::sc<type>(m_data - in.basic()) };
    }
}

#endif