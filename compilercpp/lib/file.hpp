#ifndef COMPILERCPP_LIB_FILE
#define COMPILERCPP_LIB_FILE

#include "header.hpp"

#include <iostream>
#include <fstream>
#include <string>

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

auto log_file(const std::string& in) -> void {
    get_logfile() << in;
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