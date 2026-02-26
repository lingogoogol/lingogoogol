#ifndef LGO_IO_TLS_EXTENSION_KEY_SHARE_GROUP_X25519
#define LGO_IO_TLS_EXTENSION_KEY_SHARE_GROUP_X25519

#include "pv.hpp"

LGO_EXPORT namespace lgo::tls {
    class x25519: public lgo, public group_pv {
    public:
        template<is_mem_cpt t_out>
        static auto mem_pad(const x25519& in, t_out& out) -> void;

        x25519(const curve25519& in);

        auto size(const size_mem<>&) const -> intpc;

        auto group_type() const -> intp16;
    private:
        ptrs<const curve25519> m_data{};
    };

    template<is_mem_cpt t_out>
    auto x25519::mem_pad(const x25519& in, t_out& out) -> void {
        out.pad(in.m_data.ref().public_key(), numz_little_endian{});
        return;
    }

    x25519::x25519(const curve25519& in): m_data{ &in } {}
        
    auto x25519::size(const size_mem<>&) const -> intpc {
        return m_data.ref().public_key().size(size_mem{});
    }

    auto x25519::group_type() const -> intp16 {
        return group_type::x25519;
    }
}

#endif