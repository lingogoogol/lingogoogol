#ifndef LGO_IO_TLS_HANDSHAKE_MAIN_IMPL
#define LGO_IO_TLS_HANDSHAKE_MAIN_IMPL

#include "def.hpp"

#include "../client_hello/def.hpp"
#include "../pv/def.hpp"

namespace lgo::tls {
	template<is_mem_cpt t_out>
	auto handshake::mem_pad(const handshake& in, t_out& out) -> void {
		out.pad(in.handshake_type(), int_big_endian{}).pad(U""_intp8).pad(U""_intp16);
		if ((in.handshake_type() == handshake_type::client_hello).basic()) {
			out.pad(*in.m_data.s().dc<client_hello>());
		}
		else {
			throw e_other{ U""_strmdc };
		}
		intp16 size{ out.padpos() - U"4"_intp16 };
		out.padpos(U"2"_intpc).pad(size, int_big_endian{});
		out.padpos(out.padpos() + size);
		return;
	}

	handshake::handshake(ptrm<handshake_pv>&& in): m_data{ in.sc<ptrm<handshake_pv>&&>() } {}
	
	auto handshake::handshake_type() const -> intp8 {
		return m_data.ref().handshake_type();
	}

	auto handshake::record_type() const -> intp8 {
		return record_type::handshake;
	}
}

#endif