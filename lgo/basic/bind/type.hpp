#ifndef LGO_BASIC_BIND_TYPE
#define LGO_BASIC_BIND_TYPE

namespace lgo {
    template<typename t_type>
    struct type_stu {
        using type = t_type::type;
    };
}

LGO_EXPORT namespace lgo {
    using type_bind = bind<tem<type_stu>>;
}

#endif