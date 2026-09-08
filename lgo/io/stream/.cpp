module lgo.io.stream;

import std;

namespace lgo {
    namespace {
        auto read_char(std::istream& stream) -> char {
            const auto value{ stream.get() };
            if (value == std::char_traits<char>::eof()) {
                throw std::ios_base::failure{ "unexpected end of string" };
            }
            return static_cast<char>(value);
        }

        template<typename t_string>
        auto check_length(const t_string& value) -> std::uint8_t {
            constexpr auto max_length{ std::numeric_limits<std::uint8_t>::max() };
            if (value.size() > max_length) {
                throw std::length_error{ "length-prefixed string exceeds 255 elements" };
            }
            return static_cast<std::uint8_t>(value.size());
        }
    }

    auto get_string(std::istream& istream) -> std::string {
        const auto len{ static_cast<std::uint8_t>(read_char(istream)) };
        return get_string(istream, len);
    }

    auto get_string(std::istream& istream, std::size_t len) -> std::string {
        std::string out(len, '\0');
        istream.read(out.data(), static_cast<std::streamsize>(len));
        if (!istream) {
            throw std::ios_base::failure{ "unexpected end of string" };
        }
        return out;
    }

    auto get_string_null_terminated(std::istream& istream) -> std::string {
        std::string out{};
        char current{};
        while ((current = read_char(istream)) != '\0') {
            out.push_back(current);
        }
        return out;
    }

    auto pad_string(std::ostream& dest, const std::string& in) -> void {
        pad_integer(dest, check_length(in));
        dest.write(in.data(), static_cast<std::streamsize>(in.size()));
        if (!dest) {
            throw std::ios_base::failure{ "failed to write string" };
        }
    }

    auto get_stringw(std::istream& src) -> std::wstring {
        std::wstring out{};
        const auto len{ static_cast<std::uint8_t>(read_char(src)) };
        for (std::size_t i{ 0 }; i < len; ++i) {
            out.push_back(get_integer<wchar_t>(src));
        }
        return out;
    }

    auto pad_stringw(std::ostream& dest, const std::wstring& in) -> void {
        pad_integer(dest, check_length(in));
        for (const auto character : in) {
            pad_integer<wchar_t>(dest, character);
        }
    }
}
