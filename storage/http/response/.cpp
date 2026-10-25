module storage.http.response;

import std;

http_response_t::http_response_t(std::string response)
{
    const auto line_end{ response.find("\r\n") };
    const auto protocol_end{ response.find(' ') };
    if (line_end == std::string::npos || protocol_end >= line_end || !response.starts_with("HTTP/"))
        throw std::invalid_argument{ "Invalid HTTP status line." };
    m_protocol = response.substr(0, protocol_end);
    const auto status_begin{ protocol_end + 1 };
    m_status_code = response.substr(status_begin, 3);
    if (status_begin + 3 > line_end || m_status_code.size() != 3 ||
        !std::ranges::all_of(m_status_code, [](char c) { return c >= '0' && c <= '9'; }) ||
        (status_begin + 3 < line_end && response[status_begin + 3] != ' '))
        throw std::invalid_argument{ "Invalid HTTP status code." };
    if (status_begin + 3 < line_end)
        m_status_text = response.substr(status_begin + 4, line_end - status_begin - 4);
    auto position{ line_end + 2 };
    while (true)
    {
        const auto end{ response.find("\r\n", position) };
        if (end == std::string::npos)
            throw std::invalid_argument{ "Truncated HTTP headers." };
        if (end == position)
        {
            m_body = response.substr(end + 2);
            return;
        }
        const auto colon{ response.find(':', position) };
        if (colon == std::string::npos || colon == position || colon >= end)
            throw std::invalid_argument{ "Invalid HTTP header." };
        auto value{ colon + 1 };
        while (value < end && (response[value] == ' ' || response[value] == '\t')) ++value;
        m_header[response.substr(position, colon - position)] = response.substr(value, end - value);
        position = end + 2;
    }
}

auto http_response_t::protocol_get() const -> std::string
{
    return m_protocol;
}

auto http_response_t::status_code_get() const -> std::string
{
    return m_status_code;
}

auto http_response_t::status_text_get() const -> std::string
{
    return m_status_text;
}

auto http_response_t::header_get(std::string name) const -> std::string
{
    return m_header.at(name);
}

auto http_response_t::body_get() const -> std::string
{
    return m_body;
}
