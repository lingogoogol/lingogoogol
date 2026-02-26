#ifndef LGO_BASIC_E_IMPL
#define LGO_BASIC_E_IMPL

#include "def.hpp"

namespace lgo {
	e::e(strmdc message): m_message{ message } {}

	auto e::message() const -> const strmdc& {
		return m_message;
	}
}

#endif