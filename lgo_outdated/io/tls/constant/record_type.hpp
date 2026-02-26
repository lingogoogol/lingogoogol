#ifndef LGO_IO_TLS_CONSTANT_RECORD_TYPE
#define LGO_IO_TLS_CONSTANT_RECORD_TYPE

namespace lgo::tls::record_type {
    constexpr inline intp8 invalid{ U"0"_intp8 };
    constexpr inline intp8 change_cipher_spec{ U"14"_intp8 };
    constexpr inline intp8 alert{ U"15"_intp8 };
    constexpr inline intp8 handshake{ U"16"_intp8 };
    constexpr inline intp8 application_data{ U"17"_intp8 };
}

#endif