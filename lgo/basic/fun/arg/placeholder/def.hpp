#ifndef LGO_BASIC_FUN_ARG_PLACEHOLDER_DEF
#define LGO_BASIC_FUN_ARG_PLACEHOLDER_DEF

LGO_EXPORT namespace lgo {
    template<is_intp_v_cpt t_pos>
    class fun_placeholder: public lgo {
    public:
        constexpr fun_placeholder() = default;
    };

    class fun_placeholder_other: public lgo {
    public:
        constexpr fun_placeholder_other() = default;
    };
}

#endif