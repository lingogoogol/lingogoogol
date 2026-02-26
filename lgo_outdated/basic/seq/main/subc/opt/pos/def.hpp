#ifndef LGO_BASIC_SEQ_MAIN_SUBC_OPT_POS_DEF
#define LGO_BASIC_SEQ_MAIN_SUBC_OPT_POS_DEF

LGO_EXPORT namespace lgo {
    template<is_intp_v_cpt t_pos>
    class seqsubc_pos {
    public:
        constexpr static intpc val{ t_pos::val };
        using val_v = v<val>;
    };
}

#endif