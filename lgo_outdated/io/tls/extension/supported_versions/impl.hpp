#ifndef LGO_IO_TLS_EXTENSION_SUPPORTED_VERSIONS_IMPL
#define LGO_IO_TLS_EXTENSION_SUPPORTED_VERSIONS_IMPL

#include "def.hpp"

namespace lgo::tls {
    auto supported_versions::size(const size_mem<>&) const -> intpc {
        return intp8::size(size_mem{}) + m_data.size(size_mem{ seq_plain{}, arr_for_all{} });
    }

    template<is_mem_cpt t_out>
    auto supported_versions::mem_pad(const supported_versions& in, t_out& out) -> void {
        out.pad(in.m_data.size(size_content{}).sc<intp8>(), int_big_endian{})
        .pad(in.m_data, seq_plain{}, arr_for_all{ int_big_endian{} });
        return;
    }

    template<typename t_in>
    supported_versions::supported_versions(t_in&& in): m_data{ in.f<t_in>() } {}

    auto supported_versions::extension_type() const -> intp16 {
        return extension_type::supported_versions;
    }
}

#endif