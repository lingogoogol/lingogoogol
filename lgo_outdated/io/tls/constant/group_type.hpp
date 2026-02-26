#ifndef LGO_IO_TLS_CONSTANT_GROUP_TYPE
#define LGO_IO_TLS_CONSTANT_GROUP_TYPE

namespace lgo::tls::group_type {
    constexpr inline intp16 secp256r1{ U"0017"_intp16 };
    constexpr inline intp16 secp384r1{ U"0018"_intp16 };
    constexpr inline intp16 secp521r1{ U"0019"_intp16 };
    constexpr inline intp16 x25519{ U"001D"_intp16 };
    constexpr inline intp16 x448{ U"001E"_intp16 };
    constexpr inline intp16 ffdhe2048{ U"0100"_intp16 };
    constexpr inline intp16 ffdhe3072{ U"0101"_intp16 };
    constexpr inline intp16 ffdhe4096{ U"0102"_intp16 };
    constexpr inline intp16 ffdhe6144{ U"0103"_intp16 };
    constexpr inline intp16 ffdhe8192{ U"0104"_intp16 };
}

#endif