#ifndef COMPILERCPP_LIB_IO
#define COMPILERCPP_LIB_IO

#include <iostream>
#include <cstdint>

auto get_string(std::istream& istream) -> std::string {
    std::string out{};
    std::uint8_t len{ static_cast<std::uint8_t>(istream.get()) };
    for (std::uint8_t i{ 0 }; i < len; i++) {
        out.push_back(static_cast<char>(istream.get()));
    }
    return out;
}

auto pad_string(std::ostream& dest, const std::string& in) -> void {
    for (std::uint8_t i{ 0 }; i < in.size(); ++i) {
        dest.put(in[i]);
    }
    return;
}

template<typename t_out>
auto get_integer(std::istream& src) -> t_out {
    t_out out{};
    src.read(reinterpret_cast<char*>(&out), sizeof out);
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

#endif