module;
#include <nlohmann/json.hpp>
export module storage.encode;

import std;

export
{
	auto base64url_encode(std::string in) -> std::string;
	auto base64url_decode(std::string in) -> std::string;
    auto content_zstd_available() -> bool;
    auto content_blob_encode(std::string raw, bool zstd,
                             nlohmann::json& name) -> std::string;
    auto content_blob_decode(const nlohmann::json& blob) -> std::string;
}
