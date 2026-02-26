#include <string>

#include <windows.h>

#include "file.hpp"

#include "../compilercpp/lib/common.hpp"

int main(int argc, char** argv) {
    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    std::string command{};
    auto setup{ CSV_to_map(".\\middle\\setting.csv") };
    if (argc == 1) {
        command =
            "cmd "
            "/c "
            "\""
                "\"" + setup["vcvarsall_path"] + "\" "
                "x64"
            "&&"
                "\"" + std::string{ argv[0] } + "\" "
                "0"
            "\"";
    }
    else {
        command =
            "\"" + setup["vscode_path"] + "\" "
            "\".\\source\\workspace\\lgo.code-workspace\"";
    }
    CreateProcessW(
        nullptr,
        to_wstring(command).data(),
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