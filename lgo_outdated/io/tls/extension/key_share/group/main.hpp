#ifndef LGO_IO_TLS_EXTENSION_KEY_SHARE_GROUP_MAIN
#define LGO_IO_TLS_EXTENSION_KEY_SHARE_GROUP_MAIN

#include "pv.hpp"
#include "x25519.hpp"

LGO_EXPORT namespace lgo::tls {
    class group: public lgo {
    public:
        template<is_mem_cpt t_out>
        static auto mem_pad(const group& in, t_out& out) -> void;
        
        group() = default;
        group(ptrm<group_pv>&& in);

        auto size(const size_mem<>&) const -> intpc;

        auto group_type() const -> intp16;
    private:
        ptrm<group_pv> m_data{ nullptr };
    };
}

namespace lgo::tls {
    template<is_mem_cpt t_out>
    auto group::mem_pad(const group& in, t_out& out) -> void {
        out.pad(in.group_type(), int_big_endian{}).pad(U""_intp16);
        if ((in.group_type() == group_type::x25519).basic()) {
            out.pad(*in.m_data.s().dc<x25519>());
        }
        else {
            throw e_other{ U""_strmdc };
        }
        intp16 size{ out.padpos() - U"4"_intp16 };
        out.padpos(U"2"_intpc).pad(size, int_big_endian{});
        out.padpos(out.padpos() + size);
        return;
    }

    group::group(ptrm<group_pv>&& in): m_data{ in.r() } {}

    auto group::size(const size_mem<>&) const -> intpc {
        return m_data.ref().size(size_mem{});
    }

    auto group::group_type() const -> intp16 {
        return m_data.ref().group_type();
    }
}

#endif