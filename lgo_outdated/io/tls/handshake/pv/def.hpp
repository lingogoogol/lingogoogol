#ifndef LGO_IO_TLS_HANDSHAKE_PV_DEF
#define LGO_IO_TLS_HANDSHAKE_PV_DEF

#include "decl.hpp"

namespace lgo::tls {
	class handshake_pv {
	public:
		virtual ~handshake_pv() = default;

		virtual auto handshake_type() const -> intp8 = 0;
	};
}

#endif