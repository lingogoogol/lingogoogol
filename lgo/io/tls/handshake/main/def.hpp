#ifndef LGO_IO_TLS_HANDSHAKE_MAIN_DEF
#define LGO_IO_TLS_HANDSHAKE_MAIN_DEF

#include "decl.hpp"

#include "../pv/decl.hpp"
#include "../../record/pv/def.hpp"

namespace lgo::tls {
	class handshake: public lgo, public record_pv {
	public:
		template<is_mem_cpt t_out>
		static auto mem_pad(const handshake& in, t_out& out) -> void;

		handshake(ptrm<handshake_pv>&& in);
		handshake(const handshake&) = delete;

		auto operator=(const handshake&) & -> handshake& = delete;
		
		auto handshake_type() const -> intp8;
		auto record_type() const -> intp8;
	private:
		ptrm<handshake_pv> m_data{ nullptr };
	};
}

#endif