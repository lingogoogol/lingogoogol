module storage.http.curl.multi;

import std;

import external.curl;

import lgo.io.file;
import lgo.dev.error;

import storage.http.curl.curl;
import storage.http.curl.easy;

auto api_curlm(CURLMcode code) -> void
{
    if (code != CURLM_OK)
    {
        throw lgo::error_t{ curl_multi_strerror(code) };
    }
    return;
}

auto curl_multi_t::get() -> const curl_multi_t&
{
    thread_local curl_multi_t multi{};
    return multi;
}

curl_multi_t::curl_multi_t()
: m_interface{ curl_multi_init() }
{
    if (!m_interface)
    {
        throw;
    }
    return;
}

curl_multi_t::~curl_multi_t()
{
    if (m_interface)
    {
        api_curlm(curl_multi_cleanup(m_interface));
    }
    return;
}

auto curl_multi_t::interface_get() const -> CURLM*
{
    return m_interface;
}

curl_multi_t::operator CURLM*() const
{
    return m_interface;
}

auto curl_multi_t::operator->() const -> CURLM*
{
    return m_interface;
}

auto curl_multi_t::add(curl_easy_t&& easy) -> void
{
    api_curlm(curl_multi_add_handle(m_interface, easy.interface_get()));
    easy.release();
    return;
}

auto curl_multi_t::run() -> int
{
    api_curlm(curl_multi_perform(m_interface, &m_running_count));
    int message_count{};
    while (true)
    {
        CURLMsg* message{ curl_multi_info_read(m_interface, &message_count) };
        if (message)
        {
            switch (message->msg)
            {
                case CURLMSG_DONE:
                {
                    api_curl(message->data.result);
	                long response_code{};
                    curl_easy_getinfo(message->easy_handle, CURLINFO_RESPONSE_CODE, &response_code);
                    curl_multi_remove_handle(m_interface, message->easy_handle);
                    if (response_code < 200 || response_code >= 300)
                    {
                        add(curl_easy_t{ message->easy_handle });
                    }
                    else
                    {
                        m_available.emplace(message->easy_handle);
                    }
                    break;
                }
                default:
                {
                    lgo::log_file("curl_multi_t::run: An unknown message was received.");
                }
            }
        }
        else if (message_count)
        {
            throw;
        }
        if (!message_count)
        {
            return m_running_count;
        }
    }
}

auto curl_multi_t::running_count() -> int
{
    return m_running_count;
}

auto curl_multi_t::poll(int timeout_ms) -> void
{
    api_curlm(curl_multi_poll(m_interface, nullptr, 0, timeout_ms, nullptr));
    return;
}

auto curl_multi_t::available() -> curl_easy_t
{
    if (m_available.empty())
    {
        return curl_easy_t{ nullptr };
    }
    CURL* ptr{ *m_available.begin() };
    m_available.erase(ptr);
    return curl_easy_t{ ptr };
}
