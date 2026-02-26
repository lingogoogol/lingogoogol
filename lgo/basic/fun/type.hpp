#ifndef LGO_BASIC_FUN_TYPE
#define LGO_BASIC_FUN_TYPE

#include "../type/.hpp"
#include "../seq/arr/mc/.hpp"

namespace lgo {
    template<typename t_type>
    class fun_type {
    private:
        using function = decltype(std::function{ declval<t_type>() });

        template<typename>
        struct deduce {};

        template<typename... u_arg, typename u_ret>
        struct deduce<std::function<u_ret(u_arg...)>> {
            using arg = arrmc_t<u_arg...>;
            using ret = t<u_ret>;
        };
    public:
        using arg = deduce<function>::arg;
        using arg_t = t<arg>;
        using ret = deduce<function>::ret;
        using ret_t = t<ret>;
    };
}

#endif