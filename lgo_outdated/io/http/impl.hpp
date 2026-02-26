#ifndef LGO_IO_HTTP_IMPL
#define LGO_IO_HTTP_IMPL

#include "def.hpp"

namespace lgo {
	inline strmd8 get_http_header(const strmd8& headers, const strmd8& name) {
		intpc i{ U"0"_intpc };
		while ((i < headers.len()).basic()) {
			while ((headers[i] != U"\n"_cha8).basic()) {
				++i;
			}
			++i;
			boo same{ U"1"_boo };
			for (intpc j{ U"0"_intpc }; (j < name.len()).basic(); ++j) {
				if ((i + j >= headers.len() || headers[i + j] != name[j]).basic()) {
					same = U"0"_boo;
					break;
				}
			}
			if ((!same || headers[i + name.len()] != U":"_cha8).basic()) {
				continue;
			}
			i += name.len() + U"1"_intpc;
			if ((headers[i] == U" "_cha8).basic()) {
				++i;
			}
			strmd8 out{};
			for (; (headers[i] != U"\r"_cha8 && headers[i] != U"\n"_cha8).basic(); ++i) {
				out.push(headers[i]);
			}
			return out;
		}
		throw e_other{ U""_strmdc };
	}
}

#endif