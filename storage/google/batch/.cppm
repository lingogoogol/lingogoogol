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
        constexpr static auto part_count_max() -> std::uint64_t;

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
        auto method_post() -> void;

        auto run_clear_if_full() -> void;
        virtual auto parse() -> void;
        virtual auto valid() -> bool override;
        virtual auto resume() -> void override;
        auto saved_response() -> std::string override;
        auto restore_response(std::string_view value) -> void override;
    protected:
        auto authorization_rejected() const -> bool override;
    private:
        struct part_t
        {
            http_request_t m_request{};
            http_response_t m_response{};
            bool m_valid{ false };
        };

        auto body_generate() -> void;

        std::map<std::uint64_t, part_t> m_part{};
        std::string m_boundary{};
        std::uint64_t m_id_current{};
        std::string m_body{};
        bool m_auth_rejected{};
        //The number of parts generated in `m_body` in the last call to `body_generate`.
        std::uint64_t m_count{};
    };
}
