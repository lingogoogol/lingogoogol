module;
#include <nlohmann/json.hpp>
#ifdef LGO_HAVE_ZSTD
#include <zstd.h>
#endif
module storage.encode;
import std;

auto content_bytes_per_blob(std::string_view encoding) -> std::uint64_t
{
    if (encoding == "base64url") return 18750;
    if (encoding == "cjk14") return 43750;
    throw std::runtime_error{"Unknown content_encoding; expected base64url or cjk14."};
}

auto content_zstd_available() -> bool
{
#ifdef LGO_HAVE_ZSTD
    return true;
#else
    return false;
#endif
}

auto cjk14_encode(std::string_view input) -> std::string
{
    std::string output;
    std::uint32_t bits{};
    unsigned count{};
    auto emit = [&](std::uint32_t value)
    {
        const auto cp = 0x4e00u + value;
        output += static_cast<char>(0xe0u | (cp >> 12));
        output += static_cast<char>(0x80u | ((cp >> 6) & 63));
        output += static_cast<char>(0x80u | (cp & 63));
    };
    for (unsigned char byte : input)
    {
        bits = (bits << 8) | byte;
        count += 8;
        if (count >= 14)
        {
            count -= 14;
            emit((bits >> count) & 16383);
            bits &= (1u << count) - 1;
        }
    }
    if (count) emit(bits << (14 - count));
    return output;
}

auto cjk14_decode(std::string_view input, std::size_t size) -> std::string
{
    if (size > 43750 || input.size() % 3 || input.size() / 3 != (size * 8 + 13) / 14)
        throw std::runtime_error{"CJK14 content length mismatch."};
    std::string output;
    std::uint32_t bits{};
    unsigned count{};
    for (std::size_t i{}; i < input.size(); i += 3)
    {
        const auto a = static_cast<unsigned char>(input[i]);
        const auto b = static_cast<unsigned char>(input[i+1]);
        const auto c = static_cast<unsigned char>(input[i+2]);
        const auto cp = ((a & 15u) << 12) | ((b & 63u) << 6) | (c & 63u);
        if ((a & 0xf0u) != 0xe0u || (b & 0xc0u) != 0x80u || (c & 0xc0u) != 0x80u ||
            cp < 0x4e00u || cp > 0x8dffu)
            throw std::runtime_error{"Invalid CJK14 alphabet or UTF-8."};
        bits = (bits << 14) | (cp - 0x4e00u);
        count += 14;
        while (count >= 8 && output.size() < size)
        {
            count -= 8;
            output += static_cast<char>((bits >> count) & 255);
        }
        bits &= (1u << count) - 1;
    }
    if (output.size() != size || bits)
        throw std::runtime_error{"CJK14 length mismatch or nonzero padding bits."};
    return output;
}

auto content_blob_encode(std::string raw, std::string_view encoding, bool zstd,
                         nlohmann::json& name) -> std::string
{
    if (raw.size() > content_bytes_per_blob(encoding))
        throw std::runtime_error{"Content raw slice exceeds encoding capacity."};
    name.erase("e"); name.erase("n"); name.erase("c"); name.erase("r");
#ifdef LGO_HAVE_ZSTD
    if (zstd)
    {
        std::string compressed(ZSTD_compressBound(raw.size()), '\0');
        const auto size = ZSTD_compress(compressed.data(), compressed.size(), raw.data(), raw.size(), 3);
        if (ZSTD_isError(size)) throw std::runtime_error{ZSTD_getErrorName(size)};
        if (size < raw.size())
        {
            name["c"] = "zstd";
            name["r"] = raw.size();
            compressed.resize(size);
            raw = std::move(compressed);
        }
    }
#else
    (void)zstd; // New uploads ignore the unavailable option; tagged downloads fail explicitly.
#endif
    if (encoding == "cjk14")
    {
        name["e"] = "cjk14";
        name["n"] = raw.size(); // Exact byte count AFTER optional compression.
        auto content = cjk14_encode(raw);
        if (content.size() / 3 > 25000) throw std::runtime_error{"Content character limit exceeded."};
        return content;
    }
    auto content = base64url_encode(std::move(raw));
    if (content.size() > 25000) throw std::runtime_error{"Content character limit exceeded."};
    return content;
}

namespace
{
    auto byte_length(const nlohmann::json& name, const char* key, std::uint64_t limit) -> std::size_t
    {
        const auto& value = name.at(key);
        if (!value.is_number_unsigned() || value.get<std::uint64_t>() > limit)
            throw std::runtime_error{"Invalid content byte length."};
        return value.get<std::size_t>();
    }
}

auto content_blob_decode(const nlohmann::json& blob) -> std::string
{
    // Old saved responses may only contain description; absence means legacy.
    const auto name = blob.contains("name") ? nlohmann::json::parse(blob.at("name").get<std::string>())
                                            : nlohmann::json::object();
    const auto encoding = name.value("e", std::string{"base64url"});
    const auto capacity = content_bytes_per_blob(encoding);
    const auto content = blob.at("description").get<std::string>();
    std::string raw;
    if (encoding == "cjk14") raw = cjk14_decode(content, byte_length(name, "n", capacity));
    else
    {
        if (content.size() > 25000) throw std::runtime_error{"Content character limit exceeded."};
        raw = base64url_decode(content);
        if (raw.size() > capacity) throw std::runtime_error{"Decoded content exceeds capacity."};
    }
    if (!name.contains("c")) return raw;
    if (name.at("c") != "zstd") throw std::runtime_error{"Unknown content compression."};
    const auto size = byte_length(name, "r", capacity);
#ifdef LGO_HAVE_ZSTD
    // Bound allocation, require one complete frame, reject trailing data and wrong output length.
    if (ZSTD_findFrameCompressedSize(raw.data(), raw.size()) != raw.size() ||
        ZSTD_getFrameContentSize(raw.data(), raw.size()) != size)
        throw std::runtime_error{"Invalid zstd content frame or size."};
    std::string output(size, '\0');
    const auto decoded = ZSTD_decompress(output.data(), output.size(), raw.data(), raw.size());
    if (ZSTD_isError(decoded) || decoded != size) throw std::runtime_error{"Invalid zstd content."};
    return output;
#else
    (void)size;
    throw std::runtime_error{"This build cannot decode zstd content; rebuild with zstd."};
#endif
}
