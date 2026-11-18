module;
#include <nlohmann/json.hpp>
#include "storage/blob_format.hpp"
#ifdef LGO_HAVE_ZSTD
#include <zstd.h>
#endif
module storage.encode;
import std;

namespace
{
    constexpr std::uint64_t content_size_max_base64{18750};
}

auto content_blob_encode(std::string raw, nlohmann::json& name, std::uint64_t version) -> std::string
{
    if (raw.size() > (version ? storage_format::content_capacity : content_size_max_base64))
        throw std::runtime_error{"Content raw slice exceeds capacity."};
    storage_format::version(nlohmann::json{{"version",version}});
    if (version) name["version"] = version;
    else name.erase("version");
#ifdef LGO_HAVE_ZSTD
    if (version)
    {
        std::string compressed(ZSTD_compressBound(raw.size()), '\0');
        const auto size = ZSTD_compress(compressed.data(), compressed.size(), raw.data(), raw.size(), 3);
        if (ZSTD_isError(size)) throw std::runtime_error{ZSTD_getErrorName(size)};
        compressed.resize(size);
        raw = std::move(compressed);
    }
#else
    if (version) throw std::runtime_error{"New uploads require a build with zstd."};
#endif
    auto content = base64url_encode(std::move(raw));
    if (content.size() > 25000) throw std::runtime_error{"Content character limit exceeded."};
    return content;
}

auto content_blob_decode(const nlohmann::json& blob) -> std::string
{
    // Old saved responses may only contain description; absence means legacy.
    const auto name = blob.contains("name") ? nlohmann::json::parse(blob.at("name").get<std::string>())
                                            : nlohmann::json::object();
    const auto content = blob.at("description").get<std::string>();
    if (content.size() > 25000) throw std::runtime_error{"Content character limit exceeded."};
    auto raw = base64url_decode(content);
    if (raw.size() > content_size_max_base64) throw std::runtime_error{"Decoded content exceeds capacity."};
    const auto version = storage_format::version(name);
    // The version alone determines compression. In particular, an unversioned
    // payload is always raw Base64URL, even if its bytes resemble a zstd frame.
    if (version == 0) return raw;
#ifdef LGO_HAVE_ZSTD
    // Bound allocation, require one complete frame, reject trailing data and wrong output length.
    const auto frame_size = ZSTD_getFrameContentSize(raw.data(), raw.size());
    if (frame_size > content_size_max_base64)
        throw std::runtime_error{"Invalid zstd content size."};
    const auto size = static_cast<std::size_t>(frame_size);
    if (ZSTD_findFrameCompressedSize(raw.data(), raw.size()) != raw.size() ||
        ZSTD_getFrameContentSize(raw.data(), raw.size()) != size)
        throw std::runtime_error{"Invalid zstd content frame or size."};
    std::string output(size, '\0');
    const auto decoded = ZSTD_decompress(output.data(), output.size(), raw.data(), raw.size());
    if (ZSTD_isError(decoded) || decoded != size) throw std::runtime_error{"Invalid zstd content."};
    return output;
#else
    throw std::runtime_error{"This build cannot decode zstd content; rebuild with zstd."};
#endif
}
