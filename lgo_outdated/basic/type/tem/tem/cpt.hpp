#ifndef LGO_BASIC_TYPE_TEM_TEM_CPT
#define LGO_BASIC_TYPE_TEM_TEM_CPT

#include "def.hpp"
#include "../t/cpt.hpp"

#include "../../boo/cpt_literal.hpp"

namespace lgo {
    template<typename>
    struct is_tem_cpt_stu {
        constexpr static cpt val{ U"0"_cpt };
    };

    template<template<typename...> typename t_tem>
    struct is_tem_cpt_stu<tem<t_tem>> {
        constexpr static cpt val{ U"1"_cpt };
    };
}

LGO_EXPORT namespace lgo {
    template<typename t_in>
    concept is_tem_cpt = is_tem_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_tem_t_cpt = is_t_cpt<t_in> && is_tem_cpt<typename t_in::type>;
}

#endif