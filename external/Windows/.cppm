module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

export module external.Windows;

export {
    using ::MultiByteToWideChar;
    using ::WideCharToMultiByte;
    using ::OutputDebugStringA;

    inline constexpr auto windows_cp_utf8{ CP_UTF8 };
    inline constexpr auto windows_mb_err_invalid_chars{ MB_ERR_INVALID_CHARS };
    inline constexpr auto windows_wc_err_invalid_chars{ WC_ERR_INVALID_CHARS };
}
