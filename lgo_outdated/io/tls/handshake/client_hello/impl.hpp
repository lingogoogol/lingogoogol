#ifndef LGO_IO_TLS_HANDSHAKE_CLIENT_HELLO_IMPL
#define LGO_IO_TLS_HANDSHAKE_CLIENT_HELLO_IMPL

#include "def.hpp"

namespace lgo::tls {
	template<is_mem_cpt t_out>
	auto client_hello::mem_pad(const client_hello& in, t_out& out) -> void {
		out.padpos(U"0"_intpc);
		try {
			out
			.pad(in.m_version, int_big_endian{})
			.pad(in.m_random, numz_no_sign{}, numz_big_endian{})
			.pad(in.m_session_id.size(size_content{}).sc<intp8>(), int_big_endian{})
			.pad(in.m_session_id, seq_plain{})
			.pad(in.m_cipher_suite.size(size_content{}).sc<intp16>(), int_big_endian{})
			.pad(in.m_cipher_suite, seq_plain{}, arr_for_all{ int_big_endian{} })
			.pad(in.m_compression_method.size(size_content{}).sc<intp8>(), int_big_endian{})
			.pad(in.m_compression_method, seq_plain{}, arr_for_all{ int_big_endian{} });

			intpc size_pos{ out.padpos() };
			out.pad(U""_intp16).pad(in.m_extension, seq_plain{});
			intp16 size{ out.padpos() - U"2"_intp16 - size_pos };
			out.padpos(size_pos).pad(size, int_big_endian{});
			out.padpos(out.padpos() + size);
		}
		catch (pad_not_enough) {
			intpc required_size{
				in.m_version.size(size_mem{}) +
				in.m_random.size(size_mem{ numz_no_sign{} }) +
				intp8::size(size_mem{}) +
				in.m_session_id.size(size_mem{ seq_plain{} }) +
				intp16::size(size_mem{}) +
				in.m_cipher_suite.size(size_mem{ seq_plain{} }) +
				intp8::size(size_mem{}) +
				in.m_compression_method.size(size_mem{ seq_plain{} }) +
				intp16::size(size_mem{}) +
				in.m_extension.size(size_mem{ seq_plain{} })
			};
			throw pad_not_enough{ required_size - out.len() };
		}
		return;
	}

	template<typename t_session_id, typename t_cipher_suite, typename t_compression_method>
	client_hello::client_hello(intp16 version, const numzs8<intpc::val_v<U"0">, intpc::val_v<U"20">>& random
	, t_session_id&& session_id, t_cipher_suite&& cipher_suite
	, t_compression_method&& compression_method, arrmd<extension>&& extension)
	: m_version{ version }
	, m_random{ random }
	, m_session_id{ session_id.r() }
	, m_cipher_suite{ cipher_suite.r() }
	, m_compression_method{ compression_method.r() }
	, m_extension{ extension.r() } {}

	auto client_hello::handshake_type() const -> intp8 {
		return handshake_type::client_hello;
	}
}

#endif