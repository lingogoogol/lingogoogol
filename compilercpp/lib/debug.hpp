#ifndef COMPILERCPP_LIB_DEBUG
#define COMPILERCPP_LIB_DEBUG

#include <Windows.h>

BOOL Enumresnameprocw(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam) {
    hModule, lpType, lpName, lParam;
    return true;
}

BOOL Enumrestypeprocw(HMODULE hModule, LPWSTR lpType, LONG_PTR lParam) {
    EnumResourceNamesW(nullptr, lpType, Enumresnameprocw, 0);
    hModule, lpType, lParam;
    return true;
}

#endif