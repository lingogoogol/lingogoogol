#ifndef LGO_BASIC_FUN_BIND_DEF
#define LGO_BASIC_FUN_BIND_DEF

#include "decl.hpp"
#include "cpt.hpp"
#include "type.hpp"
#include "base.hpp"
#include "arg/placeholder/cpt.hpp"

#include "../type/.hpp"
#include "../seq/arr/mc/main/cpt.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_type, is_arrmc_cpt t_arg>
    class fun: public lgo, public fun_base<t_type>::type {
    public:
        template<typename u_in, is_arrmc_cpt u_arg>
        constexpr fun(const u_in& in, const u_arg& arg)
        requires (!is_fun_cpt<u_in>);

        template<typename u_in>
        constexpr auto operator=(u_in&& in) & -> fun&
        requires is_same_cpt<rm_qcvr<u_in>, fun>;

        template<typename... u_in>
        constexpr auto operator()(u_in&&... in) const -> auto;
    private:
        template<typename t_in>
        struct is_fun_placeholder_ptr_stu {
            constexpr static boo val{ is_ptr_boo<t_in> && is_fun_placeholder_boo<typename t_in::type> };
            using type = v<val>;
        };
        using is_fun_placeholder_ptr_bind = bind<tem<is_fun_placeholder_ptr_stu>>;

        t_type m_fun{};
        t_arg m_arg{};
    };

    template<typename u_in>
    requires (!is_fun_cpt<u_in>)
    fun(const u_in& in) -> fun<u_in, arrmc<fun_placeholder_other>>;
    template<typename u_in, is_arrmc_cpt u_arg>
    requires (!is_fun_cpt<u_in>)
    fun(const u_in& in, const u_arg& arg) -> fun<u_in, u_arg>;
}

#endif