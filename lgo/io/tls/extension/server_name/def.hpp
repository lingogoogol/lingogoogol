#ifndef LGO_IO_TLS_EXTENSION_SERVER_NAME_DEF
#define LGO_IO_TLS_EXTENSION_SERVER_NAME_DEF

#include "decl.hpp"

#include "../pv/def.hpp"

namespace lgo::tls {
    class server_name: public lgo, public extension_pv {
    public:
        auto size(const size_mem<>&) const -> intpc;
        template<is_mem_cpt t_out>
        static auto mem_pad(const server_name& in, t_out& out) -> void;

        template<typename t_in>
        server_name(t_in&& in);

        auto extension_type() const -> intp16;
    private:
        arrmd<arrmc<intp8, strmd8>> m_data{};
    };
}

#endif