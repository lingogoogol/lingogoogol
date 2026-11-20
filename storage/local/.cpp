module storage.local;

import std;

import lgo.io.file;
import storage.state;

// File system paths (Windows). Each function keeps the exact call the transfer
// code used before this module existed, so journals and orderings are unchanged.
namespace storage_local
{
    auto children(const std::string& directory) -> std::vector<child_t>
    {
        std::vector<std::filesystem::path> paths;
        for (const auto& child : std::filesystem::directory_iterator{directory})
            paths.push_back(child.path());
        std::ranges::sort(paths);
        std::vector<child_t> result;
        for (const auto& path : paths)
            result.push_back({path.string(), std::filesystem::is_regular_file(path) ? kind_t::file :
                std::filesystem::is_directory(path) ? kind_t::directory : kind_t::other});
        return result;
    }

    auto exists(const std::string& path) -> bool
    {
        return std::filesystem::symlink_status(path).type() != std::filesystem::file_type::not_found;
    }

    auto size(const std::string& path) -> std::uint64_t
    {
        return std::filesystem::file_size(path);
    }

    auto modified(const std::string& path) -> std::int64_t
    {
        return std::filesystem::last_write_time(path).time_since_epoch().count();
    }

    auto name(const std::string& path) -> std::string
    {
        return std::filesystem::path{path}.filename().string();
    }

    auto display(const std::string& path) -> std::string
    {
        return path;
    }

    auto child(const std::string& directory, const std::string& name) -> std::string
    {
        return (std::filesystem::path{directory} / name).string();
    }

    auto open_read(const std::string& path) -> std::unique_ptr<std::istream>
    {
        return std::make_unique<std::ifstream>(lgo::create_ifstream(path));
    }

    auto open_write(const std::string& path) -> std::unique_ptr<std::ostream>
    {
        return std::make_unique<std::ofstream>(lgo::create_ofstream_preserve(path));
    }

    auto create(const std::string& path) -> bool
    {
        return static_cast<bool>(lgo::create_ofstream_destroy(path));
    }

    auto resize(const std::string& path, std::uint64_t size) -> void
    {
        std::filesystem::resize_file(path, size);
    }

    auto flush(const std::string& path) -> void
    {
        storage_flush_file(path);
    }

    auto create_directories(const std::string& path) -> void
    {
        std::filesystem::create_directories(path);
    }

    auto remove_all(const std::string& path) -> void
    {
        std::filesystem::remove_all(path);
    }
}
