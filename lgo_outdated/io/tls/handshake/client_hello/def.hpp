#ifndef LGO_IO_TLS_HANDSHAKE_CLIENT_HELLO_DEF
#define LGO_IO_TLS_HANDSHAKE_CLIENT_HELLO_DEF

#include "decl.hpp"

#include "../pv/def.hpp"
#include "../../extension/main/def.hpp"

namespace lgo::tls {
	class client_hello: public lgo, public handshake_pv {
	public:
		template<is_mem_cpt t_out>
		static auto mem_pad(const client_hello& in, t_out& out) -> void;

		template<typename t_session_id, typename t_cipher_suite, typename t_compression_method>
		client_hello(intp16 version, const numzs8<intpc::val_v<U"0">, intpc::val_v<U"20">>& random
		, t_session_id&& session_id, t_cipher_suite&& cipher_suite
		, t_compression_method&& compression_method, arrmd<extension>&& extension);
		client_hello(const client_hello&) = delete;

		auto operator=(const client_hello&) & -> client_hello& = delete;

		auto handshake_type() const -> intp8;
	private:
		intp16 m_version{};
		numzs8<intpc::val_v<U"0">, intpc::val_v<U"20">> m_random{};
		memmd m_session_id{};
		arrmd<intp16> m_cipher_suite{};
		arrmd<intp8> m_compression_method{};
		arrmd<extension> m_extension{};
	};
}

#endif