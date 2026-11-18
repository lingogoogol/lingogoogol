module;
#include <nlohmann/json.hpp>
#include "storage/blob_format.hpp"
export module storage.encode;

import std;

export
{
	auto base64url_encode(std::string in) -> std::string;
	auto base64url_decode(std::string in) -> std::string;
    auto content_blob_encode(std::string raw, nlohmann::json& name, std::uint64_t version = storage_format::current_version) -> std::string;
    auto content_blob_decode(const nlohmann::json& blob) -> std::string;
}
