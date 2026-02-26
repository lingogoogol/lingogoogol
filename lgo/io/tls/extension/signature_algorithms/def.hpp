#ifndef LGO_IO_TLS_EXTENSION_SIGNATURE_ALGORITHMS_DEF
#define LGO_IO_TLS_EXTENSION_SIGNATURE_ALGORITHMS_DEF

#include "decl.hpp"

#include "../pv/def.hpp"
#include "../../constant/algorithm.hpp"

namespace lgo::tls {
    class signature_algorithms: public lgo, public extension_pv {
    public:
        auto size(const size_mem<>&) const -> intpc;
        template<is_mem_cpt t_out>
        static auto mem_pad(const signature_algorithms& in, t_out& out) -> void;

        template<typename t_in>
        signature_algorithms(t_in&& in);

        auto extension_type() const -> intp16;
    private:
        arrmd<intp16> m_data{};
    };
}

#endif