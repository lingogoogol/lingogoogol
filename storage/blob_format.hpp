#pragma once
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace storage_format
{
    // One global schema version for every blob kind. Increment whenever any
    // blob schema changes; absent versions belong to the original schema.
    inline constexpr std::uint64_t current_version = 2;
    inline constexpr std::uint64_t legacy_content_capacity = 18750;
    // ZSTD_compressBound(18600) <= 18750, including incompressible input.
    inline constexpr std::uint64_t content_capacity = 18600;

    inline auto version(const nlohmann::json& name) -> std::uint64_t
    {
        if (!name.contains("version")) return 0;
        const auto& value = name.at("version");
        if (!value.is_number_integer() || (value.is_number_integer() && !value.is_number_unsigned() && value.get<std::int64_t>() < 0))
            throw std::runtime_error{"Invalid blob version."};
        const auto result = value.get<std::uint64_t>();
        if (result > current_version) throw std::runtime_error{"Unsupported blob version; update the application."};
        return result;
    }
    inline auto minimum(const nlohmann::json& name) -> std::uint64_t
    {
        const auto own = version(name);
        if (!name.contains("version_min")) return own;
        const auto value = version(nlohmann::json{{"version",name.at("version_min")}});
        if (value > own) throw std::runtime_error{"Invalid minimum blob version."};
        return value;
    }
    inline auto folder(const nlohmann::json& name) -> bool
    {
        version(name);
        // Old files have an account routing field; unlike size, this remains
        // unambiguous once folders also store their recursive byte count.
        return name.is_object() && name.contains("next") &&
            (name.contains("type") ? name.at("type") == "folder" : !name.contains("account") && !name.contains("size"));
    }
    inline auto now() -> std::int64_t
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
}
