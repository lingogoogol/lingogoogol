#ifndef LGO_BASIC_BIND_THEN
#define LGO_BASIC_BIND_THEN

namespace lgo {
    template<is_bind_cpt t_in, is_bind_cpt t_bind>
    struct then_stu {
        template<typename... u_in>
        struct bind_stu {
            using type = t_bind::template type<typename t_in::template type<u_in...>>;
        };
    };
}

LGO_EXPORT namespace lgo {
    template<is_bind_cpt t_in, is_bind_cpt t_bind>
    using then_bind = bind<tem<then_stu<t_in, t_bind>::template bind_stu>>;
}

#endif