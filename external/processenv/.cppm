module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <processenv.h>

export module external.processenv;

export
{
    using ::GetCommandLineW;
}
