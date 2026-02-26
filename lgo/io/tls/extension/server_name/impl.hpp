#ifndef LGO_IO_TLS_EXTENSION_SERVER_NAME_IMPL
#define LGO_IO_TLS_EXTENSION_SERVER_NAME_IMPL

#include "def.hpp"

namespace lgo::tls {
    auto server_name::size(const size_mem<>&) const -> intpc {
        intpc out{ U"0"_intpc };
        out += intp16::size(size_mem{});
        for (intpc i{ U"0"_intpc }; (i < m_data.len()).basic(); ++i) {
            const arrmc<intp8, strmd8>& pair{ m_data[i] };
            out += rm_qr<decltype(pair.ind<intpc::val_v<U"0">>())>::size(size_mem{});
            const strmd8& name{ pair.ind<intpc::val_v<U"1">>() };
            out += intp16::size(size_mem{}) + name.size(size_mem{ seq_plain{} });
        }
        return out;
    }

    template<is_mem_cpt t_out>
    auto server_name::mem_pad(const server_name& in, t_out& out) -> void {
        out.pad(U""_intp16);
        for (intpc i{ U"0"_intpc }; (i < in.m_data.len()).basic(); ++i) {
            const arrmc<intp8, strmd8>& pair{ in.m_data[i] };
            out.pad(pair.ind<intpc::val_v<U"0">>(), int_big_endian{});
            const strmd8& name{ pair.ind<intpc::val_v<U"1">>() };
            out.pad(name.size(size_content{}).sc<intp16>(), int_big_endian{}).pad(name, seq_plain{});
        }
        intp16 size{ out.padpos() - U"2"_intp16 };
        out.padpos(U"0"_intpc).pad(size, int_big_endian{});
        out.padpos(out.padpos() + size);
        return;
    }

    template<typename t_in>
    server_name::server_name(t_in&& in): m_data{ in.f<t_in>() } {}

    auto server_name::extension_type() const -> intp16 {
        return extension_type::server_name;
    }
}

#endif