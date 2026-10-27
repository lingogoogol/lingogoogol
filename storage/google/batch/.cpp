module;
#include <nlohmann/json.hpp>
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

google_batch_t::google_batch_t()
: google_easy_t{} {}

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
    m_auth_rejected = false;
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
    m_auth_rejected = false;
    if (!curl_easy_t::valid()) return;

    // A successful outer HTTP response can still contain a truncated or invalid
    // multipart body. Validate the entire frame before committing any subresponse;
    // otherwise an unexpected ID can grow m_part and overrun download chunk arrays.
    try
    {
        curl_header* content_type{};
        if (curl_easy_header(*this, "Content-Type", 0, CURLH_HEADER, -1, &content_type) != CURLHE_OK || !content_type)
            throw std::invalid_argument{ "Missing multipart Content-Type." };
        std::string type{ content_type->value };
        const auto parameter{ type.find("boundary=") };
        if (parameter == std::string::npos)
            throw std::invalid_argument{ "Missing multipart boundary." };
        auto boundary{ type.substr(parameter + 9) };
        if (boundary.starts_with('"'))
        {
            const auto closing{ boundary.find('"', 1) };
            if (closing == std::string::npos) throw std::invalid_argument{ "Unclosed multipart boundary." };
            boundary = boundary.substr(1, closing - 1);
        }
        else
        {
            boundary = boundary.substr(0, boundary.find_first_of("; \t\r\n"));
        }
        if (boundary.empty()) throw std::invalid_argument{ "Empty multipart boundary." };
        const auto delimiter{ "--" + boundary };
        const auto& response{ m_write->m_payload };
        std::map<std::uint64_t, http_response_t> parsed;
        auto position{ response.find(delimiter) };
        if (position == std::string::npos) throw std::invalid_argument{ "Multipart boundary not found." };
        while (true)
        {
            position += delimiter.size();
            if (response.compare(position, 2, "--") == 0) break;
            if (response.compare(position, 2, "\r\n") != 0)
                throw std::invalid_argument{ "Invalid multipart separator." };
            position += 2;
            const auto next{ response.find("\r\n" + delimiter, position) };
            const auto header_end{ response.find("\r\n\r\n", position) };
            if (next == std::string::npos || header_end == std::string::npos || header_end >= next)
                throw std::invalid_argument{ "Truncated multipart response." };
            const auto headers{ response.substr(position, header_end - position) };
            const std::string id_header{ "Content-ID: response-" };
            const auto id_position{ headers.find(id_header) };
            if (id_position == std::string::npos ||
                (id_position != 0 && headers.substr(id_position - 2, 2) != "\r\n"))
                throw std::invalid_argument{ "Missing multipart Content-ID." };
            const auto id_begin{ id_position + id_header.size() };
            const auto id_text{ headers.substr(id_begin, headers.find("\r\n", id_begin) - id_begin) };
            std::uint64_t id{};
            const auto conversion{ std::from_chars(id_text.data(), id_text.data() + id_text.size(), id) };
            if (conversion.ec != std::errc{} || conversion.ptr != id_text.data() + id_text.size())
                throw std::invalid_argument{ "Invalid multipart Content-ID." };
            const auto requested{ m_part.find(id) };
            if (requested == m_part.end() || requested->second.m_valid || parsed.contains(id))
                throw std::invalid_argument{ "Unexpected or duplicate multipart Content-ID." };
            const auto body_begin{ header_end + 4 };
            parsed.emplace(id, http_response_t{ response.substr(body_begin, next - body_begin) });
            position = next + 2;
        }
        if (parsed.size() != m_count)
            throw std::invalid_argument{ "Missing multipart subresponses." };
        for (auto& [id, response] : parsed)
        {
            auto& part{ m_part.at(id) };
            part.m_valid = response.status_code_get().starts_with('2');
            if (response.status_code_get() == "401") m_auth_rejected = true;
            part.m_response = std::move(response);
        }
    }
    catch (const std::invalid_argument& error)
    {
        // Leave pending parts invalid so the existing retry path resends them.
        // Responses from earlier successful attempts remain available.
        lgo::log_file(std::string{ "Invalid Google batch response; retrying: " } + error.what() + "\n");
    }
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
    google_easy_t::resume();
    // Consume this attempt's auth failure. A later transport/framing failure
    // must not mistake an old retained subresponse for another fresh 401.
    m_auth_rejected = false;
    body_generate();
    // The method remains POST. Replacing just the pending body preserves the
    // authentication retry budget; method_post() would start a new request.
    m_read->m_payload = m_body;
    return;
}

auto google_batch_t::authorization_rejected() const -> bool
{
    return google_easy_t::authorization_rejected() || m_auth_rejected;
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

auto google_batch_t::saved_response() -> std::string
{
    auto parts = nlohmann::json::object();
    for (const auto &[id, part] : m_part)
        parts[std::to_string(id)] = part.m_response.body_get();
    return parts.dump();
}
auto google_batch_t::restore_response(std::string_view value) -> void
{
    const auto parts = nlohmann::json::parse(value);
    if (parts.size() != m_part.size())
        throw std::runtime_error{"Saved batch size differs."};
    for (auto &[id, part] : m_part)
    {
        part.m_response =
            http_response_t{"HTTP/1.1 200 OK\r\n\r\n" + parts.at(std::to_string(id)).get<std::string>()};
        part.m_valid = true;
    }
    m_response_code = 200;
}
