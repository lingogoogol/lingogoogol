module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wincrypt.h>

export module external.wincrypt;

constexpr auto CRYPT_STRING_BASE64URI_export{ CRYPT_STRING_BASE64URI };
#undef CRYPT_STRING_BASE64URI
constexpr auto CRYPT_STRING_NOCRLF_export{ CRYPT_STRING_NOCRLF };
#undef CRYPT_STRING_NOCRLF
constexpr auto CRYPT_STRING_BASE64_export{ CRYPT_STRING_BASE64 };
#undef CRYPT_STRING_BASE64

export {
    using ::CryptBinaryToStringA;
    using ::CryptStringToBinaryA;

    constexpr auto CRYPT_STRING_BASE64URI{ CRYPT_STRING_BASE64URI_export };
    constexpr auto CRYPT_STRING_NOCRLF{ CRYPT_STRING_NOCRLF_export };
    constexpr auto CRYPT_STRING_BASE64{ CRYPT_STRING_BASE64_export };
}
