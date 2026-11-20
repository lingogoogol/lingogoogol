#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// Portable Base64URL for platforms without crypt32's CryptBinaryToStringA.
// The encoder matches the CRYPT_STRING_BASE64 fallback in storage/encode/.cpp
// byte for byte ('-'/'_' alphabet with '=' padding), which every Windows decoder
// accepts. The decoder also accepts unpadded input and, like
// CryptStringToBinaryA, skips whitespace. tests/content_codec.cpp compares both
// directions with the Windows implementation.
namespace storage_base64url
{
    inline constexpr std::string_view alphabet{
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"};

    inline auto encode(std::string_view in) -> std::string
    {
        std::string out{};
        out.reserve((in.size() + 2) / 3 * 4);
        for (std::size_t i{}; i < in.size(); i += 3)
        {
            const std::size_t count{in.size() - i < 3 ? in.size() - i : 3};
            auto group{static_cast<std::uint32_t>(static_cast<unsigned char>(in[i])) << 16};
            if (count > 1) group |= static_cast<std::uint32_t>(static_cast<unsigned char>(in[i + 1])) << 8;
            if (count > 2) group |= static_cast<std::uint32_t>(static_cast<unsigned char>(in[i + 2]));
            out.push_back(alphabet[(group >> 18) & 0x3F]);
            out.push_back(alphabet[(group >> 12) & 0x3F]);
            out.push_back(count > 1 ? alphabet[(group >> 6) & 0x3F] : '=');
            out.push_back(count > 2 ? alphabet[group & 0x3F] : '=');
        }
        return out;
    }

    // Returns nullopt for characters outside the alphabet, misplaced padding and
    // impossible lengths.
    inline auto decode(std::string_view in) -> std::optional<std::string>
    {
        constexpr auto table{[]
        {
            std::array<signed char, 256> result{};
            result.fill(-1);
            for (std::size_t i{}; i < alphabet.size(); ++i)
                result[static_cast<unsigned char>(alphabet[i])] = static_cast<signed char>(i);
            return result;
        }()};
        std::string out{};
        out.reserve(in.size() / 4 * 3);
        std::uint32_t group{};
        std::size_t digits{}, padding{};
        for (const char c : in)
        {
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
            if (c == '=')
            {
                ++padding;
                continue;
            }
            const auto value{table[static_cast<unsigned char>(c)]};
            if (value < 0 || padding) return std::nullopt;
            group = (group << 6) | static_cast<std::uint32_t>(value);
            if (++digits % 4 == 0)
            {
                out.push_back(static_cast<char>(group >> 16));
                out.push_back(static_cast<char>(group >> 8));
                out.push_back(static_cast<char>(group));
                group = 0;
            }
        }
        const auto tail{digits % 4};
        if (tail == 1 || padding > 2 || (padding && (tail + padding) % 4)) return std::nullopt;
        if (tail == 2)
            out.push_back(static_cast<char>(group >> 4));
        else if (tail == 3)
        {
            out.push_back(static_cast<char>(group >> 10));
            out.push_back(static_cast<char>(group >> 2));
        }
        return out;
    }
}
