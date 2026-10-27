module;
#include <nlohmann/json.hpp>
export module storage.kernel.runtime;
import std;
import storage.kernel.load;
import storage.google;
import storage.http;
import storage.state;
export {
    class storage_runtime_t
    {
        std::atomic_bool* m_close;
        std::mutex m_mutex;
        std::vector<std::shared_ptr<storage_session_t>> m_sessions;
        std::map<std::string, std::shared_ptr<blob_queue_t>> m_queues;
        std::map<std::string, std::shared_ptr<google_account_t>> m_accounts;
        auto queue(const std::shared_ptr<storage_session_t>& session) -> std::shared_ptr<blob_queue_t>;

      public:
        explicit storage_runtime_t(std::atomic_bool* close) : m_close{close} {}
        ~storage_runtime_t()
        {
            stop();
        }
        auto open(const storage_settings_t& settings, const std::shared_ptr<request_control_t>& control,
                  std::string expected_email = {}, std::string saved_root = {})
            -> std::shared_ptr<storage_session_t>;
        auto clear(const std::shared_ptr<storage_session_t>& session,
                   const std::shared_ptr<request_control_t>& control, std::atomic_uint64_t& progress,
                   std::atomic_uint64_t& total) -> std::string;
        auto cleared(const std::shared_ptr<storage_session_t>& session, std::string root) -> void;
        auto stop() -> void;
        auto retry_accounts() -> void;
    };
}
