module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

export module external.Windows;

using REFIID_export = REFIID;
#undef REFIID

constexpr auto S_OK_export{ S_OK };
#undef S_OK
constexpr auto CP_UTF8_export{ CP_UTF8 };
#undef CP_UTF8
constexpr auto MB_ERR_INVALID_CHARS_export{ MB_ERR_INVALID_CHARS };
#undef MB_ERR_INVALID_CHARS
constexpr auto WC_ERR_INVALID_CHARS_export{ WC_ERR_INVALID_CHARS };
#undef WC_ERR_INVALID_CHARS

export
{
    using ::BYTE;
    using ::DWORD;
    using ::ULONG;
    using ::HRESULT;
    using ::HINSTANCE;
    using ::PWSTR;
    using ::LPCSTR;
    using ::LPWSTR;
    using ::FILETIME;
    using ::SYSTEMTIME;
    
    using REFIID = REFIID_export;

    using ::MultiByteToWideChar;
    using ::WideCharToMultiByte;
    using ::OutputDebugStringW;
    using ::FileTimeToSystemTime;
    
    constexpr auto S_OK{ S_OK_export };
    constexpr auto CP_UTF8{ CP_UTF8_export };
    constexpr auto MB_ERR_INVALID_CHARS{ MB_ERR_INVALID_CHARS_export };
    constexpr auto WC_ERR_INVALID_CHARS{ WC_ERR_INVALID_CHARS_export };
}
