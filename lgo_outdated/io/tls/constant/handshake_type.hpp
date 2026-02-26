#ifndef LGO_IO_TLS_CONSTANT_HANDSHAKE_TYPE
#define LGO_IO_TLS_CONSTANT_HANDSHAKE_TYPE

namespace lgo::tls::handshake_type {
    constexpr inline intp8 client_hello{ U"01"_intp16 };
    constexpr inline intp8 server_hello{ U"02"_intp16 };
    constexpr inline intp8 new_session_ticket{ U"04"_intp16 };
    constexpr inline intp8 end_of_early_data{ U"05"_intp16 };
    constexpr inline intp8 encrypted_extensions{ U"08"_intp16 };
    constexpr inline intp8 certificate{ U"0B"_intp16 };
    constexpr inline intp8 certificate_request{ U"0D"_intp16 };
    constexpr inline intp8 certificate_verify{ U"0F"_intp16 };
    constexpr inline intp8 finished{ U"14"_intp16 };
    constexpr inline intp8 key_update{ U"18"_intp16 };
    constexpr inline intp8 message_hash{ U"FE"_intp16 };
}

#endif