#ifndef LGO_IO_TLS_RECORD_PV_DEF
#define LGO_IO_TLS_RECORD_PV_DEF

#include "decl.hpp"

namespace lgo::tls {
    class record_pv {
    public:
        virtual ~record_pv() = default;

        virtual auto record_type() const -> intp8 = 0;
    };
}

#endif