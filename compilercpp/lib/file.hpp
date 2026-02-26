#ifndef COMPILERCPP_LIB_FILE
#define COMPILERCPP_LIB_FILE

#include <iostream>
#include <fstream>

constexpr inline std::ios_base::iostate file_exception_mask{ std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit };

auto create_ofstream(const std::wstring& path, std::ios_base::openmode mode) -> std::ofstream {
    std::ofstream out{};
    out.open(path, mode);
    out.exceptions(file_exception_mask);
    return out;
}

auto create_ifstream(const std::wstring& path, std::ios_base::openmode mode) -> std::ifstream {
    std::ifstream out{};
    out.open(path, mode);
    out.exceptions(file_exception_mask);
    return out;
}

auto get_logfile() -> std::ofstream& {
    static std::ofstream logfile{};
    return logfile;
}

auto init_logfile(const std::wstring& path) -> void {
    get_logfile().open(path, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    get_logfile().exceptions(file_exception_mask);
    return;
}

auto log(const std::string& in) -> void {
    get_logfile() << in;
    return;
}

#endif