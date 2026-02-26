#ifndef LGO_IO_TLS_CONSTANT_CIPHER_SUITE
#define LGO_IO_TLS_CONSTANT_CIPHER_SUITE

namespace lgo::tls::cipher_suite {
    constexpr inline intp16 TLS_AES_128_GCM_SHA256{ U"1301"_intp16 };
    constexpr inline intp16 TLS_AES_256_GCM_SHA384{ U"1302"_intp16 };
    constexpr inline intp16 TLS_CHACHA20_POLY1305_SHA256{ U"1303"_intp16 };
    constexpr inline intp16 TLS_AES_128_CCM_SHA256{ U"1304"_intp16 };
    constexpr inline intp16 TLS_AES_128_CCM_8_SHA256{ U"1305"_intp16 };
}

#endif