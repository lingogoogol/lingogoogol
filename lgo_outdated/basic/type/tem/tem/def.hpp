#ifndef LGO_BASIC_TYPE_TEM_TEM_DEF
#define LGO_BASIC_TYPE_TEM_TEM_DEF

#include "../t/def.hpp"
#include "../v/def.hpp"
#include "../../lgo/def.hpp"

LGO_EXPORT namespace lgo {
    template<template<typename...> typename t_tem>
    class tem: public lgo {
    private:
        template<typename... u_in>
        struct type_stu {
            using type = t_tem<u_in...>;
        };
    public:
        tem() = delete;
        ~tem() = delete;

        template<typename... t_in>
        using type = t_tem<t_in...>::type;
        template<typename... t_in>
        using type_t = t<type<t_in...>>;
        template<typename... t_in>
        constexpr static auto val{ t_tem<t_in...>::val };
        template<typename... t_in>
        using val_v = v<val<t_in...>>;
    };

    template<template<typename...> typename t_tem>
    using tem_t = t<tem<t_tem>>;
}

#endif