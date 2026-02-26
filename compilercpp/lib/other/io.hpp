#ifndef COMPILERCPP_LIB_OTHER_IO
#define COMPILERCPP_LIB_OTHER_IO

#include <iostream>
#include <cstdint>

template<typename t_out>
auto get_integer(std::istream& src) -> t_out {
    t_out out{};
    src.read(reinterpret_cast<char*>(&out), sizeof out);
    return out;
}

template<typename t_out>
auto get_integer(std::istream& src, std::uint8_t size) -> t_out {
    t_out out{};
    src.read(reinterpret_cast<char*>(&out), size);
    return out;
}

template<typename t_in>
auto pad_integer(std::ostream& dest, t_in in) -> void {
    dest.write(reinterpret_cast<const char*>(&in), sizeof in);
    return;
}

template<typename t_in>
auto fill_integer(std::ostream& dest, std::streampos pos, t_in in) -> void {
    const std::streampos current_pos{ dest.tellp() };
    dest.seekp(pos);
    pad_integer(dest, in);
    dest.seekp(current_pos);
    return;
}

auto get_string(std::istream& istream) -> std::string {
    std::string out{};
    std::uint8_t len{ static_cast<std::uint8_t>(istream.get()) };
    for (std::uint8_t i{ 0 }; i < len; i++) {
        out.push_back(static_cast<char>(istream.get()));
    }
    return out;
}

auto pad_string(std::ostream& dest, const std::string& in) -> void {
    pad_integer(dest, static_cast<std::uint8_t>(in.size()));
    for (std::uint8_t i{ 0 }; i < in.size(); ++i) {
        dest.put(in[i]);
    }
    return;
}

auto get_stringw(std::istream& src) -> std::wstring {
    std::wstring out{};
    std::uint8_t len{ static_cast<std::uint8_t>(src.get()) };
    for (std::uint8_t i{ 0 }; i < len; i++) {
        out.push_back(get_integer<wchar_t>(src));
    }
    return out;
}

auto pad_stringw(std::ostream& dest, const std::wstring& in) -> void {
    pad_integer(dest, static_cast<std::uint8_t>(in.size()));
    for (std::uint8_t i{ 0 }; i < in.size(); ++i) {
        pad_integer<wchar_t>(dest, in[i]);
    }
    return;
}

#endif