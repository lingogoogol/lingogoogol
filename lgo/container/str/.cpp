module lgo.container.str;

import std;

import external.Windows;

namespace lgo {
    auto to_wstring(std::string in) -> std::wstring {
        if (in.empty()) {
            return {};
        }
        std::wstring out{};
        const int output_size{ MultiByteToWideChar(windows_cp_utf8, windows_mb_err_invalid_chars,
            in.data(), static_cast<int>(in.size()), nullptr, 0) };
        if (output_size <= 0) {
            throw std::runtime_error{ "invalid UTF-8 string" };
        }
        out.resize(static_cast<std::size_t>(output_size));
        if (MultiByteToWideChar(windows_cp_utf8, windows_mb_err_invalid_chars, in.data(),
            static_cast<int>(in.size()), out.data(), output_size) != output_size) {
            throw std::runtime_error{ "UTF-8 to UTF-16 conversion failed" };
        }
        return out;
    }

    auto to_string(std::wstring in) -> std::string {
        if (in.empty()) {
            return {};
        }
        std::string out{};
        const int output_size{ WideCharToMultiByte(windows_cp_utf8, windows_wc_err_invalid_chars,
            in.data(), static_cast<int>(in.size()), nullptr, 0, nullptr, nullptr) };
        if (output_size <= 0) {
            throw std::runtime_error{ "invalid UTF-16 string" };
        }
        out.resize(static_cast<std::size_t>(output_size));
        if (WideCharToMultiByte(windows_cp_utf8, windows_wc_err_invalid_chars, in.data(),
            static_cast<int>(in.size()), out.data(), output_size, nullptr, nullptr) != output_size) {
            throw std::runtime_error{ "UTF-16 to UTF-8 conversion failed" };
        }
        return out;
    }
}
