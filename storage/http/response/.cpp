module storage.http.response;

import std;

http_response_t::http_response_t(std::string response)
{
    std::uint64_t pos_end{ response.find(" ") };
    m_protocol = response.substr(0, pos_end);
    std::uint64_t pos_begin{ pos_end + 1 };
    pos_end = response.find(" ", pos_begin);
    m_status_code = response.substr(pos_begin, pos_end - pos_begin);
    pos_begin = pos_end + 1;
    pos_end = response.find("\r\n", pos_begin);
    m_status_text = response.substr(pos_begin, pos_end - pos_begin);
    pos_begin = pos_end + 2;
    while (true)
    {
        pos_end = response.find("\r\n", pos_begin);
        if (pos_end == pos_begin)
        {
            pos_begin = pos_end + 2;
            break;
        }
        std::string header_name{}, header_val{};
        std::uint64_t pos_colon{ response.find(": ", pos_begin) };
        header_name = response.substr(pos_begin, pos_colon - pos_begin);
        pos_colon += 2;
        header_val = response.substr(pos_colon, pos_end - pos_colon);
        m_header[header_name] = header_val;
        pos_begin = pos_end + 2;
    }
    m_body = response.substr(pos_begin);
    return;
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
