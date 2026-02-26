#ifndef LGO_IO_TLS_RECORD_PLAIN_DEF
#define LGO_IO_TLS_RECORD_PLAIN_DEF

#include "decl.hpp"

#include "../main/def.hpp"
#include "../../constant/record_type.hpp"

namespace lgo::tls {
	class record_plain: public lgo, public record {
	public:
		using record::record;

		template<is_mem_cpt t_out>
		static auto mem_pad(const record_plain& in, t_out& out) -> void;
	};
}

#endif