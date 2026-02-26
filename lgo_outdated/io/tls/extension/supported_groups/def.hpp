#ifndef LGO_IO_TLS_EXTENSION_SUPPORTED_GROUPS_DEF
#define LGO_IO_TLS_EXTENSION_SUPPORTED_GROUPS_DEF

#include "decl.hpp"

#include "../pv/def.hpp"
#include "../../constant/group_type.hpp"

namespace lgo::tls {
	class supported_groups: public lgo, public extension_pv {
	public:
        auto size(const size_mem<>&) const -> intpc;
		template<is_mem_cpt t_out>
		static auto mem_pad(const supported_groups& in, t_out& out) -> void;

		template<typename t_in>
		supported_groups(t_in&& in);

		auto extension_type() const -> intp16;
    private:
        arrmd<intp16> m_data{};
	};
}

#endif