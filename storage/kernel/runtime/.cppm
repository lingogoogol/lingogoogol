module;
#include <nlohmann/json.hpp>
export module storage.kernel.runtime;
import std;
import storage.kernel.load;
import storage.google;
import storage.http;
import storage.state;
export {
    struct service_account_file_t
    {
        // Keep only the address and credential path in the account menu.
        std::string address, account_path;
    };
    auto storage_service_accounts(const std::filesystem::path& directory = "key")
        -> std::vector<service_account_file_t>;

    class storage_runtime_t
    {
        std::atomic_bool* m_close;
        std::mutex m_mutex;
        std::vector<std::shared_ptr<storage_session_t>> m_sessions;
        std::map<std::string, std::shared_ptr<blob_queue_t>> m_queues;
        std::map<std::string, std::shared_ptr<google_account_t>> m_accounts;
        auto queue(const std::shared_ptr<storage_session_t>& session) -> std::shared_ptr<blob_queue_t>;

      public:
        explicit storage_runtime_t(std::atomic_bool* close);
        ~storage_runtime_t()
        {
            stop();
        }
        auto open(const storage_settings_t& settings, const std::shared_ptr<request_control_t>& control,
                  std::string expected_email = {}, std::string saved_root = {},
                  std::string expected_storage = {})
            -> std::shared_ptr<storage_session_t>;
        auto open_oauth(const storage_settings_t& settings,
                        std::shared_ptr<google_oauth_user_t> user,
                        const std::shared_ptr<request_control_t>& control,
                        std::string saved_root = {}) -> std::shared_ptr<storage_session_t>;
        auto clear(const std::shared_ptr<storage_session_t>& session,
                   const std::shared_ptr<request_control_t>& control, std::atomic_uint64_t& progress,
                   std::atomic_uint64_t& total) -> std::string;
        auto cleared(const std::shared_ptr<storage_session_t>& session, std::string root) -> void;
        auto stop() -> void;
        auto sign_out(const std::string& storage_id) -> void;
        // Switching the browser retains these sessions and their background tasks.
        auto signed_in_sessions() -> std::vector<std::shared_ptr<storage_session_t>>;
        auto retry_accounts() -> void;
    };
}
