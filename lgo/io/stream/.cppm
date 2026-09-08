export module lgo.io.stream;

import std;

export namespace lgo {
    struct uint128_t {
        std::uint64_t m_significant_most{};
        std::uint64_t m_significant_least{};
    };

    template<typename t_out>
    auto get_integer(std::istream& src) -> t_out {
        t_out out{};
        src.read(reinterpret_cast<char*>(&out), sizeof out);
        if (!src) {
            throw std::ios_base::failure{ "unexpected end of integer" };
        }
        return out;
    }

    template<>
    auto get_integer<uint128_t>(std::istream& src) -> uint128_t {
        uint128_t out{};
        src.read(reinterpret_cast<char*>(&out.m_significant_most), sizeof out.m_significant_most);
        src.read(reinterpret_cast<char*>(&out.m_significant_least), sizeof out.m_significant_least);
        if (!src) {
            throw std::ios_base::failure{ "unexpected end of uint128" };
        }
        return out;
    }

    template<typename t_out>
    auto get_integer(std::istream& src, std::uint8_t size) -> t_out {
        if (size > sizeof(t_out)) {
            throw std::length_error{ "encoded integer is larger than its destination type" };
        }
        t_out out{};
        src.read(reinterpret_cast<char*>(&out), size);
        if (!src) {
            throw std::ios_base::failure{ "unexpected end of integer" };
        }
        return out;
    }

    template<typename t_in>
    auto pad_integer(std::ostream& dest, t_in in) -> void {
        dest.write(reinterpret_cast<const char*>(&in), sizeof in);
        if (!dest) {
            throw std::ios_base::failure{ "failed to write integer" };
        }
    }

    template<>
    auto pad_integer<uint128_t>(std::ostream& dest, uint128_t in) -> void {
        dest.write(reinterpret_cast<const char*>(&in.m_significant_most), sizeof in.m_significant_most);
        dest.write(reinterpret_cast<const char*>(&in.m_significant_least), sizeof in.m_significant_least);
        if (!dest) {
            throw std::ios_base::failure{ "failed to write uint128" };
        }
    }

    template<typename t_in>
    auto pad_integer(std::ostream& dest, t_in in, std::uint8_t size) -> void {
        if (size > sizeof(t_in)) {
            throw std::length_error{ "requested integer width exceeds its source type" };
        }
        dest.write(reinterpret_cast<const char*>(&in), size);
        if (!dest) {
            throw std::ios_base::failure{ "failed to write integer" };
        }
    }

    template<typename t_in>
    auto fill_integer(std::ostream& dest, std::streampos pos, t_in in) -> void {
        const std::streampos current_pos{ dest.tellp() };
        dest.seekp(pos);
        pad_integer(dest, in);
        dest.seekp(current_pos);
        return;
    }

    auto get_string(std::istream& istream) -> std::string;
    auto get_string(std::istream& istream, std::size_t len) -> std::string;
    auto get_string_null_terminated(std::istream& istream) -> std::string;
    auto pad_string(std::ostream& dest, const std::string& in) -> void;
    auto get_stringw(std::istream& src) -> std::wstring;
    auto pad_stringw(std::ostream& dest, const std::wstring& in) -> void;
}
