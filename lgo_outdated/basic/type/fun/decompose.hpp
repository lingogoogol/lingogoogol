#ifndef LGO_BASIC_TYPE_FUN_DECOMPOSE
#define LGO_BASIC_TYPE_FUN_DECOMPOSE

#include "../tem/bind/.hpp"

namespace lgo {
    template<typename>
    struct decompose_stu {};

    template<template<typename...> typename t_tem, typename... t_arg>
    struct decompose_stu<t_tem<t_arg...>> {
        using bind = bind<tem<t_tem>>;
        using arg = arrmc<t_arg...>;
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    using decompose_bind = decompose_stu<t_in>::bind;
    template<typename t_in>
    using decompose_arg = decompose_stu<t_in>::arg;
}

#endif