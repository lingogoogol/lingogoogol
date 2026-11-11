export module storage.google.batch;

import std;

import external.curl;

import storage.http;
import storage.google.easy;
import storage.google.account;

export
{
    class google_batch_t: public google_easy_t
    {
    public:
        // Importers need the definition when sizing their bounded request windows.
        constexpr static auto part_count_max() -> std::uint64_t { return 100; }
        static auto gzip_available() -> bool;
        // Snapshotted by method_post(); existing attempts keep their encoding.
        static auto gzip_enabled_set(bool enabled) -> void;

        google_batch_t();
        google_batch_t(curl_easy_t&& easy);
        
        auto size() const -> std::uint64_t;
        auto clear() -> void override;
        
        auto path_set(std::string path) -> void;
        auto boundary_get() const -> std::string;
        auto boundary_set(std::string boundary) -> void;

        auto part_count() -> std::uint64_t;
        auto request_add() -> std::uint64_t;
        auto request_add_handle() -> http_request_t&;
        auto request_get(std::uint64_t id) -> http_request_t&;
        auto response_get(std::uint64_t id) -> http_response_t&;
        // Opt in per part: replayed DELETEs can legitimately return 404. A
        // caller accepting missing GETs must still inspect their status.
        auto response_allow_missing(std::uint64_t id) -> void;
        auto method_post() -> void;
        // Retain the indexed response API while sending one ordinary HTTP request.
        auto method_single() -> void;

        auto run_clear_if_full() -> void;
        virtual auto parse() -> void;
        virtual auto valid() -> bool override;
        auto error_context() const -> std::string override;
        auto failure_reason() const -> std::string override;
        virtual auto resume() -> void override;
        auto saved_response() -> std::string override;
        auto restore_response(std::string_view value) -> void override;
    protected:
        auto authorization_rejected() const -> bool override;
        auto request_body() const -> const std::string& override;
    private:
        struct part_t
        {
            http_request_t m_request{};
            http_response_t m_response{};
            bool m_valid{ false };
            bool m_allow_missing{};
        };

        auto body_generate() -> void;
        auto body_payload() -> std::string;

        std::map<std::uint64_t, part_t> m_part{};
        std::string m_boundary{};
        std::uint64_t m_id_current{};
        std::string m_body{};
        bool m_auth_rejected{};
        bool m_direct{};
        bool m_gzip{};
        std::string m_failure;
        //The number of parts generated in `m_body` in the last call to `body_generate`.
        std::uint64_t m_count{};
    };
}
