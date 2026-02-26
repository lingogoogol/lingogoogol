#ifndef LGO_IO_TLS_EXTENSION_SIGNATURE_ALGORITHMS_IMPL
#define LGO_IO_TLS_EXTENSION_SIGNATURE_ALGORITHMS_IMPL

#include "def.hpp"

namespace lgo::tls {
    auto signature_algorithms::size(const size_mem<>&) const -> intpc {
        return intp16::size(size_mem{}) + m_data.size(size_mem{ seq_plain{}, arr_for_all{} });
    }

    template<is_mem_cpt t_out>
    auto signature_algorithms::mem_pad(const signature_algorithms& in, t_out& out) -> void {
        out.pad(in.m_data.size(size_content{}).sc<intp16>(), int_big_endian{})
        .pad(in.m_data, seq_plain{}, arr_for_all{ int_big_endian{} });
        return;
    }

    template<typename t_in>
    signature_algorithms::signature_algorithms(t_in&& in): m_data{ in.f<t_in>() } {}

    auto signature_algorithms::extension_type() const -> intp16 {
        return extension_type::signature_algorithms;
    }
}

#endif