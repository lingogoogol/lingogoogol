#ifndef LGO_IO_TLS_RECORD_PLAIN_IMPL
#define LGO_IO_TLS_RECORD_PLAIN_IMPL

#include "def.hpp"

#include "../../handshake/main/def.hpp"
#include "../pv/def.hpp"

namespace lgo::tls {
	template<is_mem_cpt t_out>
	auto record_plain::mem_pad(const record_plain& in, t_out& out) -> void {
		out.pad(in.record_type(), int_big_endian{}).pad(in.m_version, int_big_endian{}).pad(U""_intp16);
		if ((in.record_type() == record_type::handshake).basic()) {
			out.pad(*in.m_data.s().dc<handshake>());
		}
		else {
			throw e_other{ U""_strmdc };
		}
		intp16 size{ out.padpos() - U"5"_intp16 };
		out.padpos(U"3"_intpc).pad(size, int_big_endian{});
		out.padpos(out.padpos() + size);
		return;
	}
}

#endif