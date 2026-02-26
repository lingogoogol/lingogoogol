#ifndef LGO_BASIC_TYPE_TEM_BIND_DECL
#define LGO_BASIC_TYPE_TEM_BIND_DECL

namespace lgo {
    template<typename t_tem, typename... t_arg>
    class bind;
    template<typename t_tem>
    class bind<t_tem>;
}

#endif