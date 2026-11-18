module;
#include <nlohmann/json.hpp>
#ifdef LGO_HAVE_ZSTD
#include <zstd.h>
#endif
module storage.encode;
import std;

namespace
{
    constexpr std::uint64_t content_size_max_base64{18750};
}

auto content_zstd_available() -> bool
{
#ifdef LGO_HAVE_ZSTD
    return true;
#else
    return false;
#endif
}

auto content_blob_encode(std::string raw, bool zstd,
                         nlohmann::json& name) -> std::string
{
    if (raw.size() > content_size_max_base64)
        throw std::runtime_error{"Content raw slice exceeds capacity."};
    name.erase("c"); name.erase("r");
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
    if (name.contains("e")) throw std::runtime_error{"Unsupported content encoding."};
    const auto content = blob.at("description").get<std::string>();
    if (content.size() > 25000) throw std::runtime_error{"Content character limit exceeded."};
    auto raw = base64url_decode(content);
    if (raw.size() > content_size_max_base64) throw std::runtime_error{"Decoded content exceeds capacity."};
    if (!name.contains("c")) return raw;
    if (name.at("c") != "zstd") throw std::runtime_error{"Unknown content compression."};
    const auto size = byte_length(name, "r", content_size_max_base64);
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
