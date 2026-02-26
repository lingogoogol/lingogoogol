#ifndef LGO_BASIC_SEQ_MAIN_SUBD_IMPL
#define LGO_BASIC_SEQ_MAIN_SUBD_IMPL

#include "def.hpp"

namespace lgo {
    template<typename t_type>
    template<is_seqsub_cpt t_self, is_seq_cpt u_in>
    constexpr auto seqsubd<t_type>::operator=(this t_self&& self, const u_in& in)
    -> t_self&& requires (!is_qc_cpt<base::type_actual>) {
        self.m_data.ref().replace(self.m_pos, self.len(), in);
        self.m_len = in.len();
        return self.f<t_self>();
    }

    template<typename t_type>
    constexpr auto seqsubd<t_type>::operator=(const seqsubd& in) & -> seqsubd& {
        return operator=<seqsubd>(in);
    }
}

#endif