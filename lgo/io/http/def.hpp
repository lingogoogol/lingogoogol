#ifndef LGO_IO_HTTP_DEF
#define LGO_IO_HTTP_DEF

#include "decl.hpp"

namespace lgo {
	enum class http_method {
		GET,
		POST,
		PUT,
		DELETE,
		HEAD,
		CONNECT,
		OPTIONS,
		TRACE
	};
}

#endif