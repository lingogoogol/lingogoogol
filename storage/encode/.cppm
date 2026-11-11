module;
#include <nlohmann/json.hpp>
export module storage.encode;

import std;

export
{
	auto base64url_encode(std::string in) -> std::string;
	auto base64url_decode(std::string in) -> std::string;
    auto content_bytes_per_blob(std::string_view encoding) -> std::uint64_t;
    auto content_zstd_available() -> bool;
    auto cjk14_encode(std::string_view input) -> std::string;
    auto cjk14_decode(std::string_view input, std::size_t size) -> std::string;
    auto content_blob_encode(std::string raw, std::string_view encoding, bool zstd,
                             nlohmann::json& name) -> std::string;
    auto content_blob_decode(const nlohmann::json& blob) -> std::string;
}
