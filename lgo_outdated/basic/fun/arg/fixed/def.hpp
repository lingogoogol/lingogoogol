#ifndef LGO_BASIC_FUN_ARG_FIXED_DEF
#define LGO_BASIC_FUN_ARG_FIXED_DEF

LGO_EXPORT namespace lgo {
    template<typename t_type>
    class fun_fixed: public lgo {
    private:
        using type = rm_qr<t_type>;
    public:
        constexpr fun_fixed(t_type&& in);

        constexpr auto data() -> t_type&&;
    private:
        ptrs<t_type> m_data{};
    };
}

#endif