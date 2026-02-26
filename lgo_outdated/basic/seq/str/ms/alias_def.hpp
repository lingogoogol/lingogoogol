#ifndef LGO_BASIC_SEQ_STR_MS_ALIAS_DEF
#define LGO_BASIC_SEQ_STR_MS_ALIAS_DEF

#include "alias_decl.hpp"
#include "def.hpp"

#include "../../../type/cha/.hpp"
#include "../../../type/lgo/.hpp"

LGO_EXPORT namespace lgo {
    template<is_intp_v_cpt t_len>
    class strms8: public strms<cha8, t_len>, public lgo {
    private:
        using base = strms<cha8, t_len>;
    public:
        using base::strms;

        constexpr auto operator=(const strms8&) & -> strms8&;
    };

    template<is_intp_v_cpt t_len>
    class strms16: public strms<cha16, t_len>, public lgo {
    private:
        using base = strms<cha16, t_len>;
    public:
        using base::strms;

        constexpr auto operator=(const strms16&) & -> strms16&;
    };

    template<is_intp_v_cpt t_len>
    class strms32: public strms<cha32, t_len>, public lgo {
    private:
        using base = strms<cha32, t_len>;
    public:
        using base::strms;

        constexpr auto operator=(const strms32&) & -> strms32&;
    };

    template<is_intp_v_cpt t_len>
    class strmsc: public strms<chac, t_len>, public lgo {
    private:
        using base = strms<chac, t_len>;
    public:
        using base::strms;

        constexpr auto operator=(const strmsc&) & -> strmsc&;
    };
}

namespace lgo {
    template<is_literal_str_cpt t_in>
    constexpr inline intpc strms_deduce_len{ t_in::len() };
}

LGO_EXPORT namespace lgo {
    template<is_cha_cpt t_in>
    strms8(const t_in&) -> strms8<intpc::val_v<U"1">>;
    template<is_literal_str_cpt t_in>
    strms8(const t_in&) -> strms8<v<strms_deduce_len<t_in>>>;
    template<is_cha_cpt t_in>
    strms16(const t_in&) -> strms16<intpc::val_v<U"1">>;
    template<is_literal_str_cpt t_in>
    strms16(const t_in&) -> strms16<v<strms_deduce_len<t_in>>>;
    template<is_cha_cpt t_in>
    strms32(const t_in&) -> strms32<intpc::val_v<U"1">>;
    template<is_literal_str_cpt t_in>
    strms32(const t_in&) -> strms32<v<strms_deduce_len<t_in>>>;
    template<is_cha_cpt t_in>
    strmsc(const t_in&) -> strmsc<intpc::val_v<U"1">>;
    template<is_literal_str_cpt t_in>
    strmsc(const t_in&) -> strmsc<v<strms_deduce_len<t_in>>>;
}

#endif