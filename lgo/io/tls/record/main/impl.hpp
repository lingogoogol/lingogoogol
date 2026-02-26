#ifndef LGO_IO_TLS_RECORD_MAIN_IMPL
#define LGO_IO_TLS_RECORD_MAIN_IMPL

#include "def.hpp"

namespace lgo::tls {
	record::record(intp16 version, ptrm<record_pv>&& in): m_version{ version }, m_data{ in.r() } {}

	auto record::record_type() const -> intp8 {
		return m_data.ref().record_type();
	}
}

#endif