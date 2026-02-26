#ifndef LGO_BASIC_IO_OPT_LEN_DEF
#define LGO_BASIC_IO_OPT_LEN_DEF

LGO_EXPORT namespace lgo {
    class io_len: public lgo {
    public:
        constexpr io_len(intpc in);

        constexpr auto len() const -> intpc;
    private:
        intpc m_len{};
    };
}

#endif