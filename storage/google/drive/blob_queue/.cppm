export module storage.google.drive.blob_queue;

import std;

import storage.google.account;
import storage.http.curl.worker;
import storage.state;

export {
    class blob_queue_t
    {
      private:
        std::deque<std::string> m_id{};
        mutable std::mutex m_mutex{};
        std::mutex m_stop_mutex;
        std::exception_ptr m_error{};
        std::thread m_thread{};
        std::condition_variable m_condition_variable{};
        std::filesystem::path m_path{};
        google_account_t* m_account{};
        const std::atomic_bool* m_close{};
        std::atomic_bool m_stopped{false};
        bool m_clearing{false};
        bool m_cleared{false};
        std::deque<std::chrono::steady_clock::time_point> m_cycle_time{};

        curl_multi_worker_t m_worker;
        std::shared_ptr<request_control_t> m_control{std::make_shared<request_control_t>()};
        auto save_locked() -> void;
        auto process() -> void;

      public:
        blob_queue_t(std::filesystem::path path, google_account_t* account, const std::atomic_bool* close);
        ~blob_queue_t();
        auto stop() -> void;
        auto check_error() const -> void;
        auto retry() -> void;
        auto control() -> std::shared_ptr<request_control_t>
        {
            return m_control;
        };

        auto get() -> std::string;
        auto get(std::uint64_t count, const std::atomic_bool* cancelled = nullptr)
            -> std::vector<std::string>;
        auto count() -> std::uint64_t;
        auto cycle_rate() const -> std::uint64_t;

        auto clear_begin() -> void;
        auto clear_end() -> void;
    };
}
