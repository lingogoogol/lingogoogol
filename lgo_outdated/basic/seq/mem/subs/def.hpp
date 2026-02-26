#ifndef LGO_BASIC_SEQ_MEM_SUBS_DEF
#define LGO_BASIC_SEQ_MEM_SUBS_DEF

#include "decl.hpp"

#include "../main/def.hpp"
#include "../../main/subd/def.hpp"

namespace lgo {
    template<typename t_type>
    class memsubs: public lgo, public seqsubs<t_type>, public seqios, public mem {
    private:
        static_assert(is_mems_cpt<rm_qcv<t_type>>);

        using base = seqsubs<t_type>;

        using base::seqsubs;

        template<is_intp_v_cpt t_len>
        friend class memms;
        friend class memmd;
        template<typename t_type>
        friend class memsubs;
    public:
        using base::operator=;
        constexpr auto operator=(const memsubs& in) & -> memsubs&;

        template<typename t_self, typename... t_opt>
        constexpr auto subcs(this t_self& self, t_opt... opt) -> memsubs<apply_qcv<const t_type, t_self>>;
        template<typename t_self, typename... t_opt>
        constexpr auto subms(this t_self& self, t_opt... opt) -> memsubs<apply_qcv<t_type, t_self>>;
    };
}

#endif