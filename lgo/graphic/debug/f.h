#ifndef LGO_DEBUG_F_H
#define LGO_DEBUG_F_H

#include <string>

#define handle_error(...) lgo::impl::handle_error1(__FILE__, __LINE__, __VA_ARGS__)
#define check_gl_error(...) lgo::impl::check_gl_error1(__FILE__, __LINE__, __VA_ARGS__)

namespace lgo {
	namespace impl {
		void handle_error1(const char* file, int line, std::u32string message);
		void check_gl_error1(const char* file, int line);
	}
}

#endif