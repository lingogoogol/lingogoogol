module storage.google.batch;

import std;

import external.curl;

import lgo.dev.error;
import lgo.io.file;

import storage.http;

google_batch_t::google_batch_t(curl_easy_t&& easy, std::string_view path)
: curl_easy_t{ std::move(easy) }
{
    host_set("https://www.googleapis.com");
    path_set("/batch" + static_cast<std::string>(path));
    boundary_set("+");
    return;
}

auto google_batch_t::size() const -> std::uint64_t
{
    return m_http.size();
}

auto google_batch_t::clear() -> void
{
    m_http.clear();
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

auto google_batch_t::request_add() -> std::uint64_t
{
    m_http[m_id_current] = http_t{};
    m_http[m_id_current].m_request.host_set("https://www.googleapis.com");
    return m_id_current++;
}

auto google_batch_t::request_add_handle() -> http_request_t&
{
    return request_get(request_add());
}

auto google_batch_t::request_get(std::uint64_t id) -> http_request_t&
{
    return m_http[id].m_request;
}

auto google_batch_t::response_get(std::uint64_t id) -> http_response_t&
{
    return m_http[id].m_response;
}

auto google_batch_t::run() -> void
{
    std::chrono::seconds sleep_duration{ 1 };
    while (true)
    {
        run_try();
        bool successful{ true };
        for (auto i : m_http)
        {
            successful = successful && i.second.m_successful;
        }
        if (successful)
        {
            break;
        }
        std::this_thread::sleep_for(sleep_duration);
        sleep_duration *= 2;
    }
    return;
}

auto google_batch_t::run_try() -> void
{
    if (!size())
    {
        return;
    }

    //Generate body.
    std::string body{};
    body += "--" + m_boundary + "\r\n";
    std::uint64_t count{ 0 };
    for (auto i : m_http)
    {
        if (i.second.m_successful)
        {
            continue;
        }
        std::string content{ i.second.m_request.to_str() };
        body += "Content-Type: application/http\r\n";
        body += "Content-Transfer-Encoding: binary\r\n";
        body += "Content-ID: " + std::to_string(i.first) + "\r\n";
        body += "\r\n" + content + "\r\n";
        body += "--" + m_boundary + "\r\n";
        ++count;
    }
    body.pop_back();
    body.pop_back();
    body += "--\r\n";
    
    //Populate headers.
    header_set("Content-Type", "multipart/mixed; boundary=" + m_boundary);
    header_set("Authorization", "Bearer " + google_account_t::default_get().get_token());

    //Send the request.
    std::string response{ method_post_run(body) };

    //Parse the response headers.
    curl_header* content_type{};
    api_curlh(curl_easy_header(*this, "Content-Type", 0, CURLH_HEADER, -1, &content_type));

    //Parse the response body.
    std::string boundary{ content_type->value };
    std::string boundary_name{ "; boundary=" };
    boundary = "--" + boundary.substr(boundary.find(boundary_name) + boundary_name.size());
    std::uint64_t pos_begin{ response.find(boundary) + boundary.size() + 2 };
    for (std::uint64_t i{ 0 }; i < count; ++i)
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
        m_http[content_id].m_response = http_response;
        m_http[content_id].m_successful = http_response.status_code_get().starts_with('2');
    }

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
