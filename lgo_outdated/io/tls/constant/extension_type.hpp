#ifndef LGO_IO_TLS_CONSTANT_EXTENSION_TYPE
#define LGO_IO_TLS_CONSTANT_EXTENSION_TYPE

namespace lgo::tls::extension_type {
    constexpr inline intp16 server_name{ U"0000"_intp16 };
    constexpr inline intp16 max_fragment_length{ U"0001"_intp16 };
    constexpr inline intp16 status_request{ U"0005"_intp16 };
    constexpr inline intp16 supported_groups{ U"000A"_intp16 };
    constexpr inline intp16 signature_algorithms{ U"000D"_intp16 };
    constexpr inline intp16 use_srtp{ U"000E"_intp16 };
    constexpr inline intp16 heartbeat{ U"000F"_intp16 };
    constexpr inline intp16 application_layer_protocol_negotiation{ U"0010"_intp16 };
    constexpr inline intp16 signed_certificate_timestamp{ U"0012"_intp16 };
    constexpr inline intp16 client_certificate_type{ U"0013"_intp16 };
    constexpr inline intp16 server_certificate_type{ U"0014"_intp16 };
    constexpr inline intp16 padding{ U"0015"_intp16 };
    constexpr inline intp16 pre_shared_key{ U"0029"_intp16 };
    constexpr inline intp16 early_data{ U"002A"_intp16 };
    constexpr inline intp16 supported_versions{ U"002B"_intp16 };
    constexpr inline intp16 cookie{ U"002C"_intp16 };
    constexpr inline intp16 psk_key_exchange_modes{ U"002D"_intp16 };
    constexpr inline intp16 certificate_authorities{ U"002F"_intp16 };
    constexpr inline intp16 oid_filters{ U"0030"_intp16 };
    constexpr inline intp16 post_handshake_auth{ U"0031"_intp16 };
    constexpr inline intp16 signature_algorithms_cert{ U"0032"_intp16 };
    constexpr inline intp16 key_share{ U"0033"_intp16 };
} 

#endif