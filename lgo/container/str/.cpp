module lgo.container.str;

import std;

import external.Windows;

namespace lgo {
    auto to_wstring(std::string in) -> std::wstring {
        std::wstring out{};
        out.resize(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, in.data(), static_cast<int>(in.size()), nullptr, 0));
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, in.data()
        , static_cast<int>(in.size()), out.data(), static_cast<int>(out.size()));
        return out;
    }

    auto to_string(std::wstring in) -> std::string {
        std::string out{};
        out.resize(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, in.data(), static_cast<int>(in.size()), nullptr, 0, 0, nullptr));
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, in.data()
        , static_cast<int>(in.size()), out.data(), static_cast<int>(out.size()), 0, nullptr);
        return out;
    }
}
