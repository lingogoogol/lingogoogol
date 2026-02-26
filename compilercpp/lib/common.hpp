#ifndef COMPILERCPP_LIB_COMMON
#define COMPILERCPP_LIB_COMMON

#include <string>
#include "header.hpp"

#include "stu.hpp"

template<typename t_in>
auto divup(t_in in1, t_in in2) -> t_in {
    return 1 + (in1 - 1) / in2;
}

template<typename t_in>
auto roundup(t_in in1, t_in in2) -> t_in {
    return divup(in1, in2) * in2;
}

auto inside(pos_2D rect_pos, size_2D rect_size, pos_2D point_pos) -> bool {
    return point_pos.x >= rect_pos.x && point_pos.x < rect_pos.x + rect_size.x
    && point_pos.y >= rect_pos.y && point_pos.y < rect_pos.y + rect_size.y;
}

auto to_wstring(std::string in) -> std::wstring {
    std::wstring out{};
    out.resize(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, in.data(), static_cast<int>(in.size()), nullptr, 0));
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, in.data()
    , static_cast<int>(in.size()), out.data(), static_cast<int>(out.size()));
    return out;
}

template<std::integral t_in>
auto to_wstring(t_in in) -> std::wstring {
    return to_wstring(in, sizeof in);
}

template<std::integral t_in>
auto to_wstring(t_in in, std::uint8_t size) -> std::wstring {
    std::wstring temp{}, out{};
    for (std::uint8_t i{ 0 }; i < size; ++i) {
        for (std::uint8_t j{ 0 }; j < 2; ++j) {
            if (in % 0x10 < 0xA) {
                temp.push_back(L'0' + in % 0x10);
            }
            else {
                temp.push_back(L'A' + in % 0x10 - 0xA);
            }
            in >>= 4;
        }
    }
    out.insert(out.begin(), temp.rbegin(), temp.rend());
    return out;
}

template<std::unsigned_integral t_out>
auto to_int(std::wstring in) -> t_out {
    return static_cast<t_out>(std::stoull(in, nullptr, 0x10));
}

auto to_string(std::wstring in) -> std::string {
    std::string out{};
    out.resize(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, in.data(), static_cast<int>(in.size()), nullptr, 0, 0, nullptr));
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, in.data()
    , static_cast<int>(in.size()), out.data(), static_cast<int>(out.size()), 0, nullptr);
    return out;
}

#endif