#ifndef COMPILERCPP_IO
#define COMPILERCPP_IO

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

auto pad_string(std::ostream& ostream, const std::string& in) -> void {
    for (std::uint8_t i{ 0 }; i < in.size(); ++i) {
        ostream.put(in[i]);
    }
    return;
}

template<typename t_in>
auto pad_integer(std::ostream& ostream, t_in in) -> void {
    ostream.write(reinterpret_cast<const char*>(&in), sizeof in);
    return;
}

template<typename t_in>
auto fill_integer(std::ostream& ostream, std::streampos pos, t_in in) -> void {
    const std::streampos current_pos{ ostream.tellp() };
    ostream.seekp(pos);
    pad_integer(ostream, in);
    ostream.seekp(current_pos);
    return;
}

#endif