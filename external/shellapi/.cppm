module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

export module external.shellapi;

export
{
    using ::CommandLineToArgvW;
}
