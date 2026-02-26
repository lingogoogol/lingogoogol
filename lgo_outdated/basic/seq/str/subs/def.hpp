#ifndef LGO_BASIC_SEQ_STR_SUBS_DEF
#define LGO_BASIC_SEQ_STR_SUBS_DEF

#include "decl.hpp"

#include "../main/def.hpp"
#include "../../main/subs/def.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_type>
    class strsubs: public lgo, public seqsubs<t_type>, public str, public seqios {
    private:
        static_assert(is_strs_cpt<rm_qcvr<t_type>>);

        using base = seqsubs<t_type>;

        using base::seqsubs;

        template<is_cha_cpt t_type, is_intp_v_cpt t_len>
        friend class strms;
        template<is_cha_cpt t_type>
        friend class strmd;
        template<typename t_type>
        friend class strsubs;
    public:
        using base::operator=;
        constexpr auto operator=(const strsubs& in) & -> strsubs&;

        template<typename t_self, typename... t_opt>
        constexpr auto subcs(this t_self& self, t_opt... opt) -> strsubs<apply_qcv<const t_type, t_self>>;
        template<typename t_self, typename... t_opt>
        constexpr auto subms(this t_self& self, t_opt... opt) -> strsubs<apply_qcv<t_type, t_self>>;
    };
}

#endif