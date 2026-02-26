#ifndef LGO_BASIC_FUN_PLAIN_DEF
#define LGO_BASIC_FUN_PLAIN_DEF

#include "decl.hpp"
#include "base.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_type>
    class fun<t_type, arrmc<fun_placeholder_other>>
    : public lgo, public fun_base<t_type>::type {
    public:
        template<typename u_in>
        constexpr fun(const u_in& in)
        requires (!is_fun_cpt<u_in>);

        template<typename u_in>
        constexpr auto operator=(u_in&& in) & -> fun&
        requires is_same_cpt<rm_qcvr<u_in>, fun>;

        template<typename... u_arg>
        constexpr auto operator()(u_arg&&... arg) const -> auto
        requires (!std::is_member_function_pointer_v<t_type>);
        template<typename u_obj, typename... u_arg>
        constexpr auto operator()(u_obj&& obj, u_arg&&... arg) const -> auto
        requires std::is_member_function_pointer_v<t_type>;
    private:
        t_type m_fun{};
    };
}

#endif