#include <stdio.h>

module storage.http.curl.easy;

import std;

import external.curl;

import lgo.dev;
import lgo.io.file;

import storage.http.curl.curl;

auto curl_easy_t::get() -> curl_easy_t&
{
    thread_local curl_easy_t easy{};
    return easy;
}

curl_easy_t::curl_easy_t()
: curl_easy_t{ curl_easy_init() } {}

curl_easy_t::curl_easy_t(CURL* in)
: m_interface{ in }, m_read{ std::make_unique<read_t>() }, m_write{ std::make_unique<write_t>() }
{
    if (!m_interface)
    {
        throw;
    }
	api_curl(curl_easy_setopt(m_interface, CURLOPT_READFUNCTION, &curl_easy_t::read_callback));
	api_curl(curl_easy_setopt(m_interface, CURLOPT_READDATA, static_cast<void*>(m_read.get())));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_SEEKFUNCTION, &curl_easy_t::seek_callback));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_SEEKDATA, static_cast<void*>(m_read.get())));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_WRITEFUNCTION, &curl_easy_t::write_callback));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_WRITEDATA, static_cast<void*>(m_write.get())));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_VERBOSE, 1L));
    #ifdef _DEBUG
    api_curl(curl_easy_setopt(m_interface, CURLOPT_DEBUGDATA, this));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_DEBUGFUNCTION, &debug_callback));
    #endif
    api_curl(curl_easy_setopt(m_interface, CURLOPT_TIMEOUT, 300L));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_CONNECTTIMEOUT, 100L));
    return;
}

curl_easy_t::curl_easy_t(curl_easy_t&& in)
:
    m_interface{ in.m_interface },
    m_path{ std::move(in.m_path) },
    m_host{ std::move(in.m_host) },
    m_query{ std::move(in.m_query) },
    m_header{ std::move(in.m_header) },
    m_read{ std::move(in.m_read) },
    m_write{ std::move(in.m_write) },
    m_header_curl{ in.m_header_curl }
{
    in.m_interface = nullptr;
    in.m_header_curl = nullptr;
    return;
}

auto curl_easy_t::operator=(curl_easy_t&& in) -> curl_easy_t&
{
    m_interface = in.m_interface;
    m_path = std::move(in.m_path);
    m_host = std::move(in.m_host);
    m_query = std::move(in.m_query);
    m_header = std::move(in.m_header);
    m_read = std::move(in.m_read);
    m_write = std::move(in.m_write);
    m_header_curl = in.m_header_curl;
    
    in.m_interface = nullptr;
    in.m_header_curl = nullptr;
    return *this;
}

curl_easy_t::~curl_easy_t()
{
    free();
    return;
}

auto curl_easy_t::free() -> void
{
    //Passing in a nullptr makes curl_easy_cleanup return immediately with no action.
    curl_easy_cleanup(m_interface);
    return;
}

auto curl_easy_t::release() -> void
{
    m_interface = nullptr;
    return;
}

auto curl_easy_t::interface_get() const -> CURL*
{
    return m_interface;
}

curl_easy_t::operator CURL*() const
{
    return m_interface;
}

auto curl_easy_t::operator->() const -> CURL*
{
    return m_interface;
}

auto curl_easy_t::nonempty() const -> bool
{
    return m_interface;
}

curl_easy_t::operator bool() const
{
    return nonempty();
}

auto curl_easy_t::clear() -> void
{
    m_host.clear();
    m_path.clear();
    m_query.clear();
    m_header.clear();
    return;
}

auto curl_easy_t::host_get() const -> std::string
{
    return m_host;
}

auto curl_easy_t::host_set(std::string host) -> void
{
    m_host = host;
    return;
}

auto curl_easy_t::path_get() const -> std::string
{
    return m_path;
}

auto curl_easy_t::path_set(std::string path) -> void
{
    m_path = path;
    return;
}

auto curl_easy_t::query_get(std::string key) const -> std::string
{
    return m_query.at(key);
}

auto curl_easy_t::query_set(std::string key, std::string val) -> void
{
    m_query[key] = val;
    return;
}

auto curl_easy_t::header_get(std::string name) const -> std::string
{
    return m_header.at(name);
}

auto curl_easy_t::header_set(std::string name, std::string val) -> void
{
    m_header[name] = val;
    return;
}

auto curl_easy_t::method_get() -> void
{
    method();
	api_curl(curl_easy_setopt(m_interface, CURLOPT_CUSTOMREQUEST, nullptr));
	api_curl(curl_easy_setopt(m_interface, CURLOPT_UPLOAD, 0L));
    //When setting CURLOPT_HTTPGET to 1, libcurl automatically sets CURLOPT_NOBODY to 0 and CURLOPT_UPLOAD to 0.
	api_curl(curl_easy_setopt(m_interface, CURLOPT_HTTPGET, 1L));
    return;
}

auto curl_easy_t::method_post(std::string payload) -> void
{
    method();
	api_curl(curl_easy_setopt(m_interface, CURLOPT_CUSTOMREQUEST, nullptr));
	api_curl(curl_easy_setopt(m_interface, CURLOPT_UPLOAD, 0L));
    //When setting CURLOPT_POST to 1, libcurl automatically sets CURLOPT_NOBODY and CURLOPT_HTTPGET to 0.
    api_curl(curl_easy_setopt(m_interface, CURLOPT_POST, 1L));
    m_read->m_progress = 0;
    m_read->m_payload = payload;
    return;
}

auto curl_easy_t::method_patch(std::string payload) -> void
{
    method();
	api_curl(curl_easy_setopt(m_interface, CURLOPT_CUSTOMREQUEST, "PATCH"));
	api_curl(curl_easy_setopt(m_interface, CURLOPT_UPLOAD, 1L));
    m_read->m_progress = 0;
    m_read->m_payload = payload;
    return;
}

auto curl_easy_t::run() -> void
{
    std::chrono::seconds sleep_duration{ 1 };
    while (true)
	{
		run_try();
        if (valid())
        {
            break;
        }
        std::this_thread::sleep_for(sleep_duration);
        sleep_duration *= 2;
	}
	return;
}

auto curl_easy_t::run_try() -> void
{
    while (true)
    {
        m_write = std::make_unique<write_t>();
        api_curl(curl_easy_setopt(m_interface, CURLOPT_WRITEDATA, static_cast<void*>(m_write.get())));
        try
        {
            api_curl(curl_easy_perform(m_interface));
		    api_curl(curl_easy_getinfo(m_interface, CURLINFO_RESPONSE_CODE, &m_response_code));
        }
        catch (lgo::error_t error)
        {
            lgo::log_file(std::string{ error.what() } + "\n");
            continue;
        }
        break;
    }
    parse();
    return;
}

auto curl_easy_t::parse() -> void {}

auto curl_easy_t::valid() -> bool
{
    return m_response_code >= 200 && m_response_code < 300;
}

auto curl_easy_t::resume() -> void
{
    m_read->m_progress = 0;
    m_write->m_progress = 0;
    m_write->m_payload.clear();
    return;
}

auto curl_easy_t::payload() -> std::string
{
    return m_write->m_payload;
}

auto curl_easy_t::debug_callback(CURL*, curl_infotype type, char* data, std::size_t size, void*) -> int
{
    lgo::log_file("curl_easy_t::debug_callback:\n");
    std::string type_str{ "unknown" };
    switch (type)
    {
        case CURLINFO_TEXT:
        {
            type_str = "text";
            break;
        }
        case CURLINFO_HEADER_OUT:
        {
            type_str = "header_out";
            break;
        }
        default: {}
    }
    lgo::log_file("type: " + type_str + "\n");
    lgo::log_file(std::string( data, size ) + "\n");
    return 0;
}

auto curl_easy_t::read_callback(char* dest, std::size_t, std::size_t size, void* src_void) -> std::size_t
{
    auto src{ static_cast<read_t*>(src_void) };
    size = std::min(size, static_cast<std::size_t>(src->m_payload.size() - src->m_progress));
    for (std::size_t i{ 0 }; i < size; ++i)
    {
        dest[i] = src->m_payload[src->m_progress + i];
    }
    src->m_progress += size;
    return size;
}

auto curl_easy_t::seek_callback(void* read_void, curl_off_t offset, int origin) -> int
{
    auto read{ static_cast<read_t*>(read_void) };
    switch (origin)
    {
        case SEEK_SET:
        {
            read->m_progress = 0;
            break;
        }
        case SEEK_CUR:
        {
            break;
        }
        case SEEK_END:
        {
            read->m_progress = read->m_payload.size();
            break;
        }
        default:
        {
            throw;
        }
    }
    read->m_progress += offset;
    return CURL_SEEKFUNC_OK;
}

auto curl_easy_t::write_callback(char* src, std::size_t, std::size_t size, void* dest_void) -> std::size_t
{
    auto dest{ static_cast<std::string*>(dest_void) };
    auto size_orig{ dest->size() };
    dest->resize(size_orig + size);
    for (std::size_t i{ 0 }; i < size; ++i)
    {
        (*dest)[size_orig + i] = src[i];
    }
    return size;
}

auto curl_easy_t::method() -> void
{
    //Set URL.
    std::string url{ m_host + m_path };
	if (!m_query.empty())
	{
		auto i{ m_query.begin() };
		url += "?" + i->first + "=" + i->second;
		++i;
		for (; i != m_query.end(); ++i)
        {
			url += "&" + i->first + "=" + i->second;
		}
	}
    api_curl(curl_easy_setopt(m_interface, CURLOPT_URL, url.c_str()));

    //Set headers.
    //Passing in a nullptr in makes curl_slist_free_all return immediately with no action.
    curl_slist_free_all(m_header_curl);
    m_header_curl = nullptr;
    for (auto i : m_header)
    {
        m_header_curl = curl_slist_append(m_header_curl, (i.first + ": " + i.second).c_str());
    }
    api_curl(curl_easy_setopt(m_interface, CURLOPT_HTTPHEADER, m_header_curl));
    return;
}
