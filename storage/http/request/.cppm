export module storage.http.request;

import std;

export
{
    class http_request_t
    {
    private:
        std::string m_method{};
        std::string m_path{};
        std::string m_host{};
        std::map<std::string, std::string> m_query{};
        std::map<std::string, std::string> m_header{};
        std::string m_body{};
    public:
        http_request_t() = default;

        auto method_get() const -> std::string;
        auto method_set(std::string method) -> void;
        auto host_get() const -> std::string;
        auto host_set(std::string host) -> void;
        auto path_get() const -> std::string;
        auto path_set(std::string path) -> void;
        auto query_get(std::string key) const -> std::string;
        auto query_set(std::string key, std::string val) -> void;
        auto header_get(std::string name) const -> std::string;
        auto header_set(std::string name, std::string val) -> void;
        auto body_get() const -> std::string;
        auto body_set(std::string body) -> void;
        
        auto to_str() const -> std::string;
    };
}
