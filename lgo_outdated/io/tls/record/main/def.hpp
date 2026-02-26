#ifndef LGO_IO_TLS_RECORD_MAIN_DEF
#define LGO_IO_TLS_RECORD_MAIN_DEF

#include "../pv/decl.hpp"

namespace lgo::tls {
    class record {
    public:
		record(intp16 version, ptrm<record_pv>&& in);
		record(const record&) = delete;

		auto operator=(const record&) & -> record& = delete;
		
		auto record_type() const -> intp8;
	protected:
		intp16 m_version{};
		ptrm<record_pv> m_data{ nullptr };
    };
}

#endif