#ifndef LGO_BASIC_SEQ_IO_D_DEF
#define LGO_BASIC_SEQ_IO_D_DEF

#include "../s/def.hpp"

namespace lgo {
    class seqiod: public seqios {
    public:
		template<typename t_self, typename t_in, typename... t_param>
		auto pad_resize(this t_self&& self, const t_in& in, t_param... param) -> t_self&&;
    };
}

#endif