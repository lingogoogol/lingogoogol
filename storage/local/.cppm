export module storage.local;

import std;

// Local files read by uploads and written by downloads. On desktop a path is a
// file system path. On Android it is a Storage Access Framework document URI
// returned by the system picker, or "<directory URI>|<relative/path>" for a
// download target below a picked folder that may not exist yet.
// Implementations: .cpp (std::filesystem), android.cpp (JNI to LocalFiles.kt).
export namespace storage_local
{
    enum class kind_t { file, directory, other };
    struct child_t
    {
        std::string path;
        kind_t kind{};
    };

    // Every entry inside `directory` in a stable order; uploads skip `other`.
    auto children(const std::string& directory) -> std::vector<child_t>;
    auto exists(const std::string& path) -> bool;
    auto size(const std::string& path) -> std::uint64_t;
    // A change stamp stored in task journals to detect modified upload sources.
    auto modified(const std::string& path) -> std::int64_t;
    // The entry's own name (a file system file name or the document's display name).
    auto name(const std::string& path) -> std::string;
    // The path as shown to the user (unchanged on desktop; readable names on Android).
    auto display(const std::string& path) -> std::string;
    // A path for `name` inside `directory`.
    auto child(const std::string& directory, const std::string& name) -> std::string;

    auto open_read(const std::string& path) -> std::unique_ptr<std::istream>;
    // Opens for writing at any offset, creating the file and keeping its content.
    auto open_write(const std::string& path) -> std::unique_ptr<std::ostream>;
    // Creates an empty file, replacing an existing one.
    auto create(const std::string& path) -> bool;
    auto resize(const std::string& path, std::uint64_t size) -> void;
    // Makes written content durable.
    auto flush(const std::string& path) -> void;
    auto create_directories(const std::string& path) -> void;
    auto remove_all(const std::string& path) -> void;
}
