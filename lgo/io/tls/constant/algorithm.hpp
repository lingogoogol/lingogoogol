#ifndef LGO_IO_TLS_CONSTANT_ALGORITHM
#define LGO_IO_TLS_CONSTANT_ALGORITHM

namespace lgo::tls::algorithm {
    constexpr inline intp16 rsa_pkcs1_sha256{ U"0401"_intp16 };
    constexpr inline intp16 rsa_pkcs1_sha384{ U"0501"_intp16 };
    constexpr inline intp16 rsa_pkcs1_sha512{ U"0601"_intp16 };
    constexpr inline intp16 ecdsa_secp256r1_sha256{ U"0403"_intp16 };
    constexpr inline intp16 ecdsa_secp384r1_sha384{ U"0503"_intp16 };
    constexpr inline intp16 ecdsa_secp512r1_sha512{ U"0603"_intp16 };
    constexpr inline intp16 rsa_pss_rsae_sha256{ U"0804"_intp16 };
    constexpr inline intp16 rsa_pss_rsae_sha384{ U"0805"_intp16 };
    constexpr inline intp16 rsa_pss_rsae_sha512{ U"0806"_intp16 };
    constexpr inline intp16 ed25519{ U"0807"_intp16 };
    constexpr inline intp16 ed448{ U"0808"_intp16 };
    constexpr inline intp16 rsa_pss_pss_sha256{ U"0809"_intp16 };
    constexpr inline intp16 rsa_pss_pss_sha384{ U"080A"_intp16 };
    constexpr inline intp16 rsa_pss_pss_sha512{ U"080B"_intp16 };
    constexpr inline intp16 rsa_pkcs1_sha1{ U"0201"_intp16 };
    constexpr inline intp16 ecdsa_sha1{ U"0203"_intp16 };
}

#endif