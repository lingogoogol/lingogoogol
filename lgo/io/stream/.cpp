module lgo.io.stream;

import std;

namespace lgo {
    auto get_string(std::istream& istream) -> std::string {
        std::string out{};
        std::uint8_t len{ static_cast<std::uint8_t>(istream.get()) };
        for (std::uint8_t i{ 0 }; i < len; i++) {
            out.push_back(static_cast<char>(istream.get()));
        }
        return out;
    }

    auto get_string(std::istream& istream, std::size_t len) -> std::string {
        std::string out{};
        for (std::uint8_t i{ 0 }; i < len; i++) {
            out.push_back(static_cast<char>(istream.get()));
        }
        return out;
    }

    auto get_string_null_terminated(std::istream& istream) -> std::string {
        std::string out{};
        char current{};
        while ((current = static_cast<char>(istream.get()))) {
            out.push_back(current);
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
}
