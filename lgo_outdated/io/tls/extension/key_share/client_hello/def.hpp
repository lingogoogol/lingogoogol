#ifndef LGO_IO_TLS_EXTENSION_KEY_SHARE_CLIENT_HELLO_DEF
#define LGO_IO_TLS_EXTENSION_KEY_SHARE_CLIENT_HELLO_DEF

#include "../group/.hpp"

LGO_EXPORT namespace lgo::tls {
    class key_share_client_hello: public lgo, public extension_pv {
    public:
        template<is_mem_cpt t_out>
        static auto mem_pad(const key_share_client_hello& in, t_out& out) -> void;

        template<typename t_in>
        key_share_client_hello(t_in&& in);

        auto size(const size_mem<>&) const -> intpc;

        auto extension_type() const -> intp16;
    private:
        arrmd<group> m_data{};
    };
}

#endif