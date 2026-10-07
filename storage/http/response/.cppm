export module storage.http.response;

import std;

export
{
    class http_response_t
    {
    private:
        std::string m_protocol{};
        std::string m_status_code{};
        std::string m_status_text{};
        std::map<std::string, std::string> m_header{};
        std::string m_body{};
    public:
        http_response_t() = default;
        http_response_t(std::string response);
        
        auto protocol_get() const -> std::string;
        auto status_code_get() const -> std::string;
        auto status_text_get() const -> std::string;
        auto header_get(std::string name) const -> std::string;
        auto body_get() const -> std::string;
    };
}
