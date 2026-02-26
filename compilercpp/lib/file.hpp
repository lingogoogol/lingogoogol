#ifndef COMPILERCPP_LIB_FILE
#define COMPILERCPP_LIB_FILE

#include "header.hpp"

#include <iostream>
#include <fstream>
#include <string>

constexpr inline std::ios_base::iostate file_exception_mask{ std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit };
const std::string default_logfile_path{ "logfile.txt" };

auto create_ofstream(const std::wstring& path, std::ios_base::openmode mode = std::ios_base::out | std::ios_base::binary | std::ios_base::trunc) -> std::ofstream {
    std::ofstream out{};
    out.open(path, mode);
    if (!out.fail()) {
        out.exceptions(file_exception_mask);
    }
    return out;
}

auto create_ifstream(const std::wstring& path, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::binary) -> std::ifstream {
    std::ifstream out{};
    out.open(path, mode);
    if (!out.fail()) {
        out.exceptions(file_exception_mask);
    }
    return out;
}

auto get_logfile() -> std::ofstream& {
    static std::ofstream logfile{};
    return logfile;
}

auto log_file(const std::string& in) -> void {
    for (std::size_t i{ 0 }; i < in.size(); ++i) {
        get_logfile().put(in[i]);
    }
    return;
}

auto init_logfile(PWSTR arg) -> void {
    int argc{};
    LPWSTR* argv{ CommandLineToArgvW(arg, &argc) };
    if (argc >= 1) {
        get_logfile().open(argv[0]);
    }
    else {
        get_logfile().open(default_logfile_path);
    }
    bool bad_filename{ false };
    if (get_logfile().fail()) {
        get_logfile().open(default_logfile_path);
        bad_filename = true;
    }
    get_logfile().exceptions(file_exception_mask);
    if (argc > 1) {
        log_file("command_line_argument_count\n");
    }
    if (bad_filename) {
        log_file("logfile_path\n");
    }
    return;
}

auto log_console(const std::string& in) -> void {
    OutputDebugStringA(in.data());
    return;
}

template<typename t_in>
auto log_ref_count(t_in in) -> void {
    in->AddRef();
    log_console(std::to_string(in->Release()));
    return;
}

auto log_blob(Microsoft::WRL::ComPtr<ID3DBlob> in) -> void {
    for (SIZE_T i{ 0 }; i < in->GetBufferSize(); ++i) {
        log_file(std::string{ *(reinterpret_cast<const char*>(in->GetBufferPointer()) + i) });
    }
    return;
}

#endif