#ifndef LGO_BASIC_SEQ_MAIN_SUBC_OPT_LEN_DEF
#define LGO_BASIC_SEQ_MAIN_SUBC_OPT_LEN_DEF

LGO_EXPORT namespace lgo {
    template<is_intp_v_cpt t_len>
    class seqsubc_len {
    public:
        constexpr static intpc val{ t_len::val };
        using val_v = v<val>;
    };
}

#endif