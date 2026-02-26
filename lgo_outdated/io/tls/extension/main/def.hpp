#ifndef LGO_IO_TLS_EXTENSION_MAIN_DEF
#define LGO_IO_TLS_EXTENSION_MAIN_DEF

#include "decl.hpp"

#include "../pv/decl.hpp"

namespace lgo::tls {
    class extension: public lgo {
    public:
        auto size(const size_mem<>&) const -> intpc;
        template<is_mem_cpt t_out>
        static auto mem_pad(const extension& in, t_out& out) -> void;

        extension() = default;
        extension(ptrm<extension_pv>&& in);

        auto extension_type() const -> intp16;
    private:
        ptrm<extension_pv> m_data{ nullptr };
    };
}

#endif