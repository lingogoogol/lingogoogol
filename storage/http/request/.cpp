module storage.http.request;

import std;

auto http_request_t::method_get() const -> std::string
{
    return m_method;
}

auto http_request_t::method_set(std::string method) -> void
{
    m_method = method;
    return;
}

auto http_request_t::host_get() const -> std::string
{
    return m_host;
}

auto http_request_t::host_set(std::string host) -> void
{
    m_host = host;
    return;
}

auto http_request_t::path_get() const -> std::string
{
    return m_path;
}

auto http_request_t::path_set(std::string path) -> void
{
    m_path = path;
    return;
}

auto http_request_t::query_get(std::string key) const -> std::string
{
    return m_query.at(key);
}

auto http_request_t::query_set(std::string key, std::string val) -> void
{
    m_query[key] = val;
    return;
}

auto http_request_t::header_get(std::string name) const -> std::string
{
    return m_header.at(name);
}

auto http_request_t::header_set(std::string name, std::string val) -> void
{
    m_header[name] = val;
    return;
}

auto http_request_t::body_get() const -> std::string
{
    return m_body;
}

auto http_request_t::body_set(std::string body) -> void
{
    m_body = body;
    return;
}

auto http_request_t::to_str() const -> std::string
{
    std::string out{};
    out += m_method + " " + m_path;
    if (!m_query.empty())
    {
        auto i{ m_query.begin() };
        out += "?" + i->first + "=" + i->second;
        ++i;
        for (; i != m_query.end(); ++i)
        {
            out += "&" + i->first + "=" + i->second;
        }
    }
    out += " HTTP/1.1\r\n";
    // m_host stores an origin URL, but an HTTP Host field contains only its
    // authority. Batch subrequests must also retain their JSON Content-Type.
    std::string_view authority{m_host};
    if (authority.starts_with("https://")) authority.remove_prefix(8);
    else if (authority.starts_with("http://")) authority.remove_prefix(7);
    authority = authority.substr(0, authority.find_first_of("/?#"));
    out += "Host: " + std::string{authority} + "\r\n";
    for (const auto& [name, value] : m_header)
    {
        auto lower{name};
        std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return std::tolower(c); });
        // Generate framing fields once, from this request's actual body/origin.
        if (lower != "host" && lower != "content-length")
            out += name + ": " + value + "\r\n";
    }
    out += "Content-Length: " + std::to_string(m_body.size()) + "\r\n";
    out += "\r\n" + m_body;
    return out;
}
