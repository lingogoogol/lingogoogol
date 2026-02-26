#include <string>

#include <windows.h>

#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

int main(int argc, char** argv) {
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    std::string command{};
    if (argc == 1) {
        command =
            "cmd "
            "/c "
            "\""
                "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat\" "
                "x64"
            "&&"
                "\"" + std::string{ argv[0] } + "\" "
                "0"
            "\"";
    }
    else {
        command =
            "\"C:\\Users\\lingo\\AppData\\Local\\Programs\\Microsoft VS Code\\Code.exe\" "
            "\"./source/workspace/lgo.code-workspace\"";
    }
    CreateProcessA(
        nullptr,
        command.data(),
        nullptr,
        nullptr,
        false,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}