#ifndef LGO_IO_TLS_EXTENSION_KEY_SHARE_CLIENT_HELLO_IMPL
#define LGO_IO_TLS_EXTENSION_KEY_SHARE_CLIENT_HELLO_IMPL

#include "def.hpp"

namespace lgo::tls {
    auto key_share_client_hello::size(const size_mem<>&) const -> intpc {
        return intp16::size(size_mem{}) + m_data.size(size_mem{ seq_plain{} });
    }

    template<is_mem_cpt t_out>
    auto key_share_client_hello::mem_pad(const key_share_client_hello& in, t_out& out) -> void {
        out.pad(in.m_data.size(size_mem{ seq_plain{} }).sc<intp16>()).pad(in.m_data, seq_plain{});
        return;
    }

    template<typename t_in>
    key_share_client_hello::key_share_client_hello(t_in&& in): m_data{ in.f<t_in>() } {}

    auto key_share_client_hello::extension_type() const -> intp16 {
        return extension_type::key_share;
    }
}

#endif