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
: m_interface{ curl_easy_init() }
{
    if (!m_interface)
    {
        throw;
    }
    api_curl(curl_easy_setopt(m_interface, CURLOPT_VERBOSE, 1L));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_DEBUGDATA, this));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_DEBUGFUNCTION, &debug_callback));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_CONNECTTIMEOUT, 30L));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_LOW_SPEED_TIME, 30L));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_LOW_SPEED_LIMIT, 30L));
    return;
}

curl_easy_t::curl_easy_t(CURL* in)
: m_interface{ in } {}

curl_easy_t::curl_easy_t(curl_easy_t&& in)
:
    m_interface{ in.m_interface },
    m_path{ std::move(in.m_path) },
    m_host{ std::move(in.m_host) },
    m_query{ std::move(in.m_query) },
    m_header{ std::move(in.m_header) },
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

auto curl_easy_t::method_post(std::string body) -> void
{
    std::string url{ m_host + m_path };
	if (!m_query.empty())
	{
		auto i{ m_query.begin() };
		url += "?" + i->first + "=" + i->second;
		++i;
		for (; i != m_query.end(); ++i) {
			url += "&" + i->first + "=" + i->second;
		}
	}
    api_curl(curl_easy_setopt(m_interface, CURLOPT_URL, url.c_str()));
    //Passing in a nullptr in makes curl_slist_free_all return immediately with no action.
    curl_slist_free_all(m_header_curl);
    m_header_curl = nullptr;
    for (auto i : m_header)
    {
        m_header_curl = curl_slist_append(m_header_curl, (i.first + ": " + i.second).c_str());
    }
    api_curl(curl_easy_setopt(m_interface, CURLOPT_HTTPHEADER, m_header_curl));

    api_curl(curl_easy_setopt(m_interface, CURLOPT_POST, 1L));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size())));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_COPYPOSTFIELDS, body.data()));
    return;
}

auto curl_easy_t::method_post_run(std::string body) -> std::string
{
	std::string result{};
	long response_code{};
	do
	{
		result = method_post_run_try(body);
		api_curl(curl_easy_getinfo(m_interface, CURLINFO_RESPONSE_CODE, &response_code));
	}
	while (response_code < 200 || response_code >= 300);
	return result;
}

auto curl_easy_t::method_post_run_try(std::string body) -> std::string
{
    method_post(body);
    std::string response{};
    api_curl(curl_easy_setopt(m_interface, CURLOPT_WRITEDATA, static_cast<void*>(&response)));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_WRITEFUNCTION, &curl_easy_t::write_callback));
    while (true)
    {
        try
        {
            api_curl(curl_easy_perform(m_interface));
        }
        catch (lgo::error_t error)
        {
            lgo::log_file(std::string{ error.what() } + "\n");
            continue;
        }
        break;
    }
    return response;
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
    lgo::log_file(std::string( data, size ));
    return 0;
}

auto curl_easy_t::read_callback(char* dest, std::size_t, std::size_t size, void* src_void) -> std::size_t
{
    auto src{ static_cast<read_src_t*>(src_void) };
    size = std::min(size, static_cast<std::size_t>(src->m_data.size() - src->m_progress));
    for (std::size_t i{ 0 }; i < size; ++i)
    {
        dest[i] = src->m_data[src->m_progress + i];
    }
    src->m_progress += size;
    return size;
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
