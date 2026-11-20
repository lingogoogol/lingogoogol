module lgo.container.str;

import std;

#if defined(_WIN32)
import external.Windows;
#endif

namespace lgo
{
#if defined(_WIN32)
    auto to_wstring(std::string in) -> std::wstring
    {
        std::wstring out{};
        out.resize
        (
            MultiByteToWideChar
            (
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                in.data(),
                static_cast<int>(in.size()),
                nullptr,
                0
            )
        );
        MultiByteToWideChar
        (
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            in.data(),
            static_cast<int>(in.size()),
            out.data(),
            static_cast<int>(out.size())
        );
        return out;
    }

    auto to_string(std::wstring in) -> std::string
    {
        std::string out{};
        out.resize
        (
            WideCharToMultiByte
            (
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                in.data(),
                static_cast<int>(in.size()),
                nullptr,
                0,
                0,
                nullptr
            )
        );
        WideCharToMultiByte
        (
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            in.data(),
            static_cast<int>(in.size()),
            out.data(),
            static_cast<int>(out.size()),
            0,
            nullptr
        );
        return out;
    }
#else
    // Outside Windows, wchar_t is 32 bits and holds one Unicode code point.
    // Like MB_ERR_INVALID_CHARS / WC_ERR_INVALID_CHARS above, any invalid input
    // (malformed or overlong UTF-8, surrogates, values above U+10FFFF) yields an
    // empty result instead of replacement characters.
    auto to_wstring(std::string in) -> std::wstring
    {
        std::wstring out{};
        out.reserve(in.size());
        for (std::size_t i{}; i < in.size();)
        {
            const auto lead{ static_cast<unsigned char>(in[i]) };
            const std::size_t length{ lead < 0x80 ? 1u : (lead >> 5) == 0x6 ? 2u : (lead >> 4) == 0xE ? 3u :
                                      (lead >> 3) == 0x1E ? 4u : 0u };
            if (!length || i + length > in.size()) return {};
            char32_t code{ length == 1 ? lead : static_cast<char32_t>(lead & (0x7F >> length)) };
            for (std::size_t j{ 1 }; j < length; ++j)
            {
                const auto next{ static_cast<unsigned char>(in[i + j]) };
                if ((next & 0xC0) != 0x80) return {};
                code = (code << 6) | (next & 0x3F);
            }
            constexpr char32_t minimum[]{ 0, 0, 0x80, 0x800, 0x10000 };
            if (code < minimum[length] || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) return {};
            out.push_back(static_cast<wchar_t>(code));
            i += length;
        }
        return out;
    }

    auto to_string(std::wstring in) -> std::string
    {
        std::string out{};
        out.reserve(in.size());
        for (const auto character : in)
        {
            const auto code{ static_cast<char32_t>(character) };
            if (code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) return {};
            if (code < 0x80)
                out.push_back(static_cast<char>(code));
            else if (code < 0x800)
            {
                out.push_back(static_cast<char>(0xC0 | (code >> 6)));
                out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            }
            else if (code < 0x10000)
            {
                out.push_back(static_cast<char>(0xE0 | (code >> 12)));
                out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            }
            else
            {
                out.push_back(static_cast<char>(0xF0 | (code >> 18)));
                out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            }
        }
        return out;
    }
#endif
}
