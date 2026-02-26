#ifndef LGO_BASIC_TYPE_DEDUCE_COND_BASIC
#define LGO_BASIC_TYPE_DEDUCE_COND_BASIC

namespace lgo {
    template<bool, typename t_true, typename>
    struct tcond_basic_stu {
        using type = t_true;
    };

    template<typename t_true, typename t_false>
    struct tcond_basic_stu<false, t_true, t_false> {
        using type = t_false;
    };
    
    template<bool t_cond, typename t_true, typename t_false>
    using tcond_basic = tcond_basic_stu<t_cond, t_true, t_false>::type;
}

#endif