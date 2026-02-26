#ifndef LGO_IO_TLS_EXTENSION_SUPPORTED_GROUPS_IMPL
#define LGO_IO_TLS_EXTENSION_SUPPORTED_GROUPS_IMPL

#include "def.hpp"

namespace lgo::tls {
    auto supported_groups::size(const size_mem<>&) const -> intpc {
        return intp16::size(size_mem{}) + m_data.size(size_mem{ seq_plain{}, arr_for_all{} });
    }
    
    template<is_mem_cpt t_out>
    auto supported_groups::mem_pad(const supported_groups& in, t_out& out) -> void {
        out.pad(in.m_data.size(size_content{}).sc<intp16>(), int_big_endian{})
        .pad(in.m_data, seq_plain{}, arr_for_all{ int_big_endian{} });
        return;
    }

    template<typename t_in>
    supported_groups::supported_groups(t_in&& in): m_data{ in.f<t_in>() } {}

    auto supported_groups::extension_type() const -> intp16 {
        return extension_type::supported_groups;
    }
}

#endif