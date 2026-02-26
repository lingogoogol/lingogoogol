#ifndef LGO_BASIC_SEQ_MAIN_SUBD_DEF
#define LGO_BASIC_SEQ_MAIN_SUBD_DEF

#include "../subs/def.hpp"
#include "../../../type/tem/t/.hpp"

namespace lgo {
    template<typename t_type>
    class seqsubd: public seqsubs<t_type>, public seqd {
    private:
        using base = seqsubs<t_type>;
    protected:
        using base::seqsubs;
    public:
        template<is_seqsub_cpt t_self, is_seq_cpt u_in>
        constexpr auto operator=(this t_self&& self, const u_in& in) -> t_self&& requires (!is_qc_cpt<base::type_actual>);
        constexpr auto operator=(const seqsubd& in) & -> seqsubd&;
    };
}

#endif