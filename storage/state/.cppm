module;
#include <nlohmann/json.hpp>
export module storage.state;
import std;

export {
    // All snapshots are replaced atomically, after flushing their temporary file.
    auto storage_atomic_write(const std::filesystem::path& path, std::string_view content) -> void;
    auto storage_flush_file(const std::filesystem::path& path) -> void;
    auto storage_read(const std::filesystem::path& path) -> std::string;
    auto storage_digest(std::string_view content) -> std::string;

    struct storage_settings_t
    {
        std::string logfile_path{"logfile"};
        std::string font_path{"font/NotoSansTC-VariableFont_wght.ttf"};
        std::string account_path{"key.json"};
        // Directory containing one root ID file per storage identity.
        std::string root_path{"root"};
        int network_retries{-1};
        static auto load() -> storage_settings_t;
        auto save() const -> void;
    };

    struct request_control_t
    {
        std::atomic_bool cancelled{};
        std::atomic_bool paused{};
        // User pause is independent of an exhausted network retry budget.
        std::atomic_bool user_paused{};
        std::atomic_uint pause_epoch{};
        std::atomic_int retry_limit{-1};
        std::atomic_uint epoch{};
        mutable std::mutex mutex;
        std::string error;
        std::set<std::uintptr_t> sockets;
        auto cancel() -> void;
        auto pause(bool value) -> void;
        auto socket_open(int family, int type, int protocol) -> std::uintptr_t;
        auto socket_close(std::uintptr_t socket) -> int;
        auto fail(std::string reason) -> void;
        auto retry() -> void;
        auto reason() const -> std::string;
    };

    // A task records its intent and ID reservations before remote mutations.
    // Successful responses are kept in task.data so a crash can replay the
    // scheduler without re-transferring completed content or duplicating IDs.
    class task_journal_t
    {
        std::filesystem::path m_path;
        mutable std::mutex m_mutex;
        nlohmann::json m_data;
        bool m_finished{};
        auto save_locked() -> void;

      public:
        task_journal_t(std::filesystem::path path, nlohmann::json intent);
        auto data() const -> nlohmann::json;
        auto path() const -> const std::filesystem::path& { return m_path; }
        auto set(std::string key, nlohmann::json value) -> void;
        auto allocate(std::string key, std::size_t count,
                      const std::function<std::vector<std::string>(std::size_t)>& reserve)
            -> std::vector<std::string>;
        auto response(std::string_view key) const -> std::optional<std::string>;
        auto response(std::string_view key, std::string_view value) -> void;
        auto finish() -> void;
        static auto restore() -> std::vector<std::shared_ptr<task_journal_t>>;
        static auto create(nlohmann::json intent) -> std::shared_ptr<task_journal_t>;
    };
}
