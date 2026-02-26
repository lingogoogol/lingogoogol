#ifndef LGO_BASIC_FUN_PLAIN_IMPL
#define LGO_BASIC_FUN_PLAIN_IMPL

#include "plain_def.hpp"

namespace lgo {
    template<typename t_type>
    template<typename u_in>
    constexpr fun<t_type, arrmc<fun_placeholder_other>>::fun(const u_in& in)
    requires (!is_fun_cpt<u_in>): m_fun{ in } {}

    template<typename t_type>
    template<typename u_in>
    constexpr auto fun<t_type, arrmc<fun_placeholder_other>>::operator=(u_in&& in) & -> fun& requires is_same_cpt<rm_qcvr<u_in>, fun> {
        m_fun = ::lgo::f<u_in>(in.m_fun);
        return *this;
    }

    template<typename t_type>
    template<typename... u_arg>
    constexpr auto fun<t_type, arrmc<fun_placeholder_other>>::operator()(u_arg&&... arg) const -> auto
    requires (!std::is_member_function_pointer_v<t_type>) {
        return m_fun(::lgo::f<u_arg>(arg)...);
    }

    template<typename t_type>
    template<typename u_obj, typename... u_arg>
    constexpr auto fun<t_type, arrmc<fun_placeholder_other>>::operator()(u_obj&& obj, u_arg&&... arg) const -> auto
    requires std::is_member_function_pointer_v<t_type> {
        return (::lgo::f<u_obj>(obj).*m_fun)(::lgo::f<u_arg>(arg)...);
    }
}

#endif