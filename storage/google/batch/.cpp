module storage.google.batch;

import std;

import external.curl;

import lgo.dev.error;
import lgo.io.file;

import storage.http;
import storage.google.easy;

constexpr auto google_batch_t::part_count_max() -> std::uint64_t
{
    return 100;
}

google_batch_t::google_batch_t(curl_easy_t&& easy)
: google_easy_t{ std::move(easy) }
{
    boundary_set("+");
    return;
}

auto google_batch_t::size() const -> std::uint64_t
{
    return m_part.size();
}

auto google_batch_t::clear() -> void
{
    google_easy_t::clear();
    m_part.clear();
    m_id_current = 0;
    return;
}

auto google_batch_t::path_set(std::string path) -> void
{
    google_easy_t::path_set("/batch" + static_cast<std::string>(path));
    return;
}

auto google_batch_t::boundary_get() const -> std::string
{
    return m_boundary;
}

auto google_batch_t::boundary_set(std::string boundary) -> void
{
    m_boundary = boundary;
    return;
}

auto google_batch_t::part_count() -> std::uint64_t
{
    return m_part.size();
}

auto google_batch_t::request_add() -> std::uint64_t
{
    m_part[m_id_current] = part_t{};
    m_part[m_id_current].m_request.host_set("https://www.googleapis.com");
    return m_id_current++;
}

auto google_batch_t::request_add_handle() -> http_request_t&
{
    return request_get(request_add());
}

auto google_batch_t::request_get(std::uint64_t id) -> http_request_t&
{
    return m_part[id].m_request;
}

auto google_batch_t::response_get(std::uint64_t id) -> http_response_t&
{
    return m_part[id].m_response;
}

auto google_batch_t::method_post() -> void
{
    if (!size())
    {
        throw;
    }

    body_generate();
    header_set("Content-Type", "multipart/mixed; boundary=" + m_boundary);

    curl_easy_t::method_post(m_body);
    return;
}

auto google_batch_t::run_clear_if_full() -> void
{
    if (size() >= 100)
    {
        run();
        clear();
    }
    return;
}

auto google_batch_t::parse() -> void
{
    std::string& response{ m_write->m_payload };

    //Get the boundary between parts.
    curl_header* content_type{};
    api_curlh
    (
        curl_easy_header
        (
            *this,
            "Content-Type",
            0,
            CURLH_HEADER,
            -1,
            &content_type
        )
    );
    std::string boundary{ content_type->value };
    std::string boundary_name{ "; boundary=" };
    boundary = "--" + boundary.substr(boundary.find(boundary_name) + boundary_name.size());

    //Parse the response body.
    std::uint64_t pos_begin{ response.find(boundary) + boundary.size() + 2 };
    for (std::uint64_t i{ 0 }; i < m_count; ++i)
    {
        std::string content_id_name{ "Content-ID: response-" };
        std::uint64_t content_id_begin{ response.find(content_id_name, pos_begin) + content_id_name.size() };
        std::uint64_t content_id_end{ response.find("\r\n", content_id_begin) };
        std::uint64_t content_id{ std::stoull(response.substr(content_id_begin, content_id_end - content_id_begin)) };
        pos_begin = response.find("\r\n\r\n", pos_begin) + 2;
        std::uint64_t pos_end{ response.find(boundary, pos_begin) };
        std::string content{ response.substr(pos_begin, pos_end - pos_begin) };
        pos_begin = pos_end + boundary.size() + 2;
        http_response_t http_response{ content };
        m_part[content_id].m_response = http_response;
        m_part[content_id].m_valid = http_response.status_code_get().starts_with('2');
    }
    return;
}

auto google_batch_t::valid() -> bool
{
    if (!curl_easy_t::valid())
    {
        return false;
    }
    for (auto i : m_part)
    {
        if (!i.second.m_valid)
        {
            return false;
        }
    }
    return true;
}

auto google_batch_t::resume() -> void
{
    body_generate();
    return;
}

auto google_batch_t::body_generate() -> void
{
    m_body = "--" + m_boundary + "\r\n";
    m_count = 0;
    for (auto i : m_part)
    {
        if (i.second.m_valid)
        {
            continue;
        }
        std::string content{ i.second.m_request.to_str() };
        m_body += "Content-Type: application/http\r\n";
        m_body += "Content-Transfer-Encoding: binary\r\n";
        m_body += "Content-ID: " + std::to_string(i.first) + "\r\n";
        m_body += "\r\n" + content + "\r\n";
        m_body += "--" + m_boundary + "\r\n";
        ++m_count;
    }
    m_body.pop_back();
    m_body.pop_back();
    m_body += "--\r\n";
    return;
}
