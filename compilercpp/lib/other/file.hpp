#ifndef COMPILERCPP_LIB_OTHER_FILE
#define COMPILERCPP_LIB_OTHER_FILE

#include "header.hpp"

#include <iostream>
#include <fstream>
#include <string>

constexpr inline std::ios_base::iostate file_exception_mask{ std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit };

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

auto create_fstream(const std::wstring& path, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out | std::ios_base::binary) -> std::fstream {
    std::fstream out{};
    out.open(path, std::ios_base::out | std::ios_base::app);
    out.close();
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

auto get_logfile_string() -> std::string& {
    static std::string logfile_string{};
    return logfile_string;
}

auto log_file(const std::string& in) -> void {
    if (get_logfile().is_open()) {
        for (std::size_t i{ 0 }; i < in.size(); ++i) {
            get_logfile().put(in[i]);
        }
    }
    else {
        get_logfile_string() += in;
    }
    return;
}

auto init_logfile(const std::wstring& path) -> bool {
    get_logfile().open(path);
    if (get_logfile().fail()) {
        return false;
    }
    get_logfile().exceptions(file_exception_mask);
    log_file(get_logfile_string());
    return true;
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