#ifndef LGO_BASIC_SEQ_STR_MS_DEF
#define LGO_BASIC_SEQ_STR_MS_DEF

#include "../main/def.hpp"
#include "../../arr/ms/.hpp"
#include "../../io/s/def.hpp"
#include "../../../type/cha/.hpp"
#include "../../../type/int/p/def.hpp"

namespace lgo {
    template<is_cha_cpt t_type, is_intp_v_cpt t_len>
    class strms: public seqms<t_type, t_len>, public str, public seqios {
    private:
        using base = seqms<t_type, t_len>;
    public:
        using base::seqms;
        template<is_literal_str_cpt t_in>
        constexpr strms(const t_in& in);

        using base::operator=;
        constexpr auto operator=(const strms& in) & -> strms&;

        template<typename t_self, typename... t_opt>
        constexpr auto subcs(this t_self& self, t_opt... opt) -> strsubs<const t_self>;
        template<typename t_self, typename... t_opt>
        constexpr auto subms(this t_self& self, t_opt... opt) -> strsubs<t_self>;
    };
}

#endif