module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

export module external.Windows;

export {
    using ::MultiByteToWideChar;
    using ::WideCharToMultiByte;
}
