module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

export module external.Windows;

constexpr auto CP_UTF8_export{ CP_UTF8 };
#undef CP_UTF8
constexpr auto MB_ERR_INVALID_CHARS_export{ MB_ERR_INVALID_CHARS };
#undef MB_ERR_INVALID_CHARS
constexpr auto WC_ERR_INVALID_CHARS_export{ WC_ERR_INVALID_CHARS };
#undef WC_ERR_INVALID_CHARS

export {
    using ::BYTE;
    using ::DWORD;
    using ::LPCSTR;

    using ::MultiByteToWideChar;
    using ::WideCharToMultiByte;
    using ::OutputDebugStringW;
    
    constexpr auto CP_UTF8{ CP_UTF8_export };
    constexpr auto MB_ERR_INVALID_CHARS{ MB_ERR_INVALID_CHARS_export };
    constexpr auto WC_ERR_INVALID_CHARS{ WC_ERR_INVALID_CHARS_export };
}
