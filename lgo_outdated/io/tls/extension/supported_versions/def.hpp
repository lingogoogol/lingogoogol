#ifndef LGO_IO_TLS_EXTENSION_SUPPORTED_VERSIONS_DEF
#define LGO_IO_TLS_EXTENSION_SUPPORTED_VERSIONS_DEF

#include "decl.hpp"

#include "../pv/def.hpp"
#include "../../constant/version.hpp"

namespace lgo::tls {
	class supported_versions: public lgo, public extension_pv {
	public:
        auto size(const size_mem<>&) const -> intpc;
		template<is_mem_cpt t_out>
		static auto mem_pad(const supported_versions& in, t_out& out) -> void;

		template<typename t_in>
		supported_versions(t_in&& in);

		auto extension_type() const -> intp16;
	private:
		arrmd<intp16> m_data{};
	};
}

#endif