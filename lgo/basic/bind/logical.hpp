#ifndef LGO_BASIC_BIND_LOGICAL
#define LGO_BASIC_BIND_LOGICAL

#include "../type/tem/.hpp"

namespace lgo {
    template<is_bind_cpt...>
    struct and_bind_stu {};

    template<>
    struct and_bind_stu<> {
        template<typename... t_in>
        struct bind_stu {
            using type = boo::val_v<U"1">;
        };
    };

    template<is_bind_cpt t_first, is_bind_cpt... t_other>
    struct and_bind_stu<t_first, t_other...> {
        template<typename... t_in>
        struct bind_stu {
            constexpr static boo val{ t_first::template type<t_in...>::val
            && and_bind_stu<t_other...>::template bind_stu<t_in...>::type::val };
            using type = v<val>;
        };
    };
}

LGO_EXPORT namespace lgo {
    template<is_bind_cpt... t_bind>
    using and_bind = bind<tem<and_bind_stu<t_bind...>::template bind_stu>>;
}

namespace lgo {
    template<is_bind_cpt...>
    struct or_bind_stu {};

    template<>
    struct or_bind_stu<> {
        template<typename... t_in>
        struct bind_stu {
            using type = boo::val_v<U"0">;
        };
    };
    
    template<is_bind_cpt t_first, is_bind_cpt... t_other>
    struct or_bind_stu<t_first, t_other...> {
        template<typename... t_in>
        struct bind_stu {
            constexpr static boo val{ t_first::template type<t_in...>::val
            || or_bind_stu<t_other...>::template bind_stu<t_in...>::type::val };
            using type = v<val>;
        };
    };
}

LGO_EXPORT namespace lgo {
    template<is_bind_cpt... t_bind>
    using or_bind = bind<tem<or_bind_stu<t_bind...>::template bind_stu>>;
}

#endif