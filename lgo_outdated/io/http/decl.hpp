#ifndef LGO_IO_HTTP_DECL
#define LGO_IO_HTTP_DECL

namespace lgo {
	enum class http_method;

	strmdc get_http_header(const strmdc& headers, const strmdc& name);
}

#endif