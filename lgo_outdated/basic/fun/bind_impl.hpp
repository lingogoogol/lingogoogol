#ifndef LGO_BASIC_FUN_BIND_IMPL
#define LGO_BASIC_FUN_BIND_IMPL

#include "bind_def.hpp"

namespace lgo {
    template<typename t_type, is_arrmc_cpt t_arg>
    template<typename u_in, is_arrmc_cpt u_arg>
    constexpr fun<t_type, t_arg>::fun(const u_in& in, const u_arg& arg)
    requires (!is_fun_cpt<u_in>): m_fun{ in }, m_arg{ arg } {}

    template<typename t_type, is_arrmc_cpt t_arg>
    template<typename u_in>
    constexpr auto fun<t_type, t_arg>::operator=(u_in&& in) & -> fun&
    requires is_same_cpt<rm_qcvr<u_in>, fun> {
        m_fun = ::lgo::f<u_in>(in.m_fun);
        m_arg = in.m_arg.f<u_in>();
        return *this;
    }

    template<typename t_type, is_arrmc_cpt t_arg>
    template<typename... u_in>
    constexpr auto fun<t_type, t_arg>::operator()(u_in&&... in) const -> auto {
        return m_arg.each(fun{
            [] <typename t_in> (t_in&& in) -> ptrs<t_in> {
                return ::lgo::f<t_in>(&in);
            }
        }).replace<is_fun_placeholder_ptr_bind>(arrmc{ &::lgo::f<u_in>(in)... }).apply(fun{ m_fun });
    }
}

#endif