#ifndef LGO_IO_TLS_EXTENSION_MAIN_IMPL
#define LGO_IO_TLS_EXTENSION_MAIN_IMPL

#include "def.hpp"

#include "../server_name/def.hpp"
#include "../supported_groups/def.hpp"
#include "../signature_algorithms/def.hpp"
#include "../supported_versions/def.hpp"
#include "../key_share/client_hello/def.hpp"
#include "../pv/def.hpp"

namespace lgo::tls {
    auto extension::size(const size_mem<>&) const -> intpc {
        return rm_qr<decltype(extension_type())>::size(size_mem{}) + intp16::size(size_mem{}) + m_data.ref().size(size_mem{});
    }

    template<is_mem_cpt t_out>
    auto extension::mem_pad(const extension& in, t_out& out) -> void {
        out.pad(in.extension_type(), int_big_endian{}).pad(U""_intp16);
        if ((in.extension_type() == extension_type::server_name).basic()) {
            out.pad(*in.m_data.s().dc<server_name>());
        }
        else if ((in.extension_type() == extension_type::supported_groups).basic()) {
            out.pad(*in.m_data.s().dc<supported_groups>());
        }
        else if ((in.extension_type() == extension_type::signature_algorithms).basic()) {
            out.pad(*in.m_data.s().dc<signature_algorithms>());
        }
        else if ((in.extension_type() == extension_type::supported_versions).basic()) {
            out.pad(*in.m_data.s().dc<supported_versions>());
        }
        else if ((in.extension_type() == extension_type::key_share).basic()) {
            if ((in.m_data.s().dc<key_share_client_hello>() != ptrs<key_share_client_hello>{ nullptr }).basic()) {
                out.pad(*in.m_data.s().dc<key_share_client_hello>());
            }
            else {
                throw e_other{ U""_strmdc };
            }
        }
        else {
            throw e_other{ U""_strmdc };
        }
        intp16 size{ out.padpos() - U"4"_intp16 };
        out.padpos(U"2"_intpc).pad(size, int_big_endian{});
        out.padpos(out.padpos() + size);
        return;
    }

    extension::extension(ptrm<extension_pv>&& in): m_data{ in.r() } {}

    auto extension::extension_type() const -> intp16 {
        return m_data.ref().extension_type();
    }
}

#endif