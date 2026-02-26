#ifndef LGO_IO_TLS_EXTENSION_KEY_SHARE_GROUP_PV
#define LGO_IO_TLS_EXTENSION_KEY_SHARE_GROUP_PV

namespace lgo::tls {
    class group_pv {
    public:
        virtual ~group_pv() = default;

        virtual auto size(const size_mem<>&) const -> intpc = 0;

        virtual auto group_type() const -> intp16 = 0;
    };
}

#endif