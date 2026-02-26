#ifndef LGO_IO_TLS_EXTENSION_PV_DEF
#define LGO_IO_TLS_EXTENSION_PV_DEF

#include "decl.hpp"

namespace lgo::tls {
	class extension_pv {
	public:
		virtual ~extension_pv() = default;

        virtual auto size(const size_mem<>&) const -> intpc = 0;

		virtual auto extension_type() const -> intp16 = 0;
	};
}

#endif