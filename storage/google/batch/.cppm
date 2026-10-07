export module storage.google.batch;

import std;

import external.curl;

import storage.http;
import storage.google.account;

export
{
    class google_batch_t: public curl_easy_t
    {
    private:
        struct http_t
        {
            http_request_t m_request{};
            http_response_t m_response{};
            bool m_successful{};
        };

        std::map<std::uint64_t, http_t> m_http{};
        std::string m_boundary{};
        std::uint64_t m_id_current{};
    public:
        google_batch_t(curl_easy_t&& easy, std::string_view path);
        
        auto size() const -> std::uint64_t;
        auto clear() -> void;
        
        auto boundary_get() const -> std::string;
        auto boundary_set(std::string boundary) -> void;

        auto request_add() -> std::uint64_t;
        auto request_add_handle() -> http_request_t&;
        auto request_get(std::uint64_t id) -> http_request_t&;
        auto response_get(std::uint64_t id) -> http_response_t&;
        auto run() -> void;
        auto run_try() -> void;
        auto run_clear_if_full() -> void;
    };
}
