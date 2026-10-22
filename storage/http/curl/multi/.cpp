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

auto curl_multi_t::get() -> curl_multi_t&
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
        for (auto& added : m_added)
        {
            if (!m_ready.contains(added.first))
            {
                curl_multi_remove_handle(m_interface, added.first);
            }
        }
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

auto curl_multi_t::run() -> std::uint64_t
{
    api_curlm(curl_multi_perform(m_interface, &m_running_count));
    int message_count{};
    std::uint64_t failed_count{ 0 };
    while (true)
    {
        CURLMsg* message{ curl_multi_info_read(m_interface, &message_count) };
        if (message)
        {
            switch (message->msg)
            {
                case CURLMSG_DONE:
                {
                    std::unique_ptr<curl_easy_t>& easy_ptr{ m_added[message->easy_handle] };
                    bool successful{ true };
                    bool handle_removed{ false };
                    try
                    {
                        api_curl(message->data.result);
                        curl_multi_remove_handle(m_interface, message->easy_handle);
                        handle_removed = true;
                        api_curl
                        (
                            curl_easy_getinfo
                            (
                                message->easy_handle,
                                CURLINFO_RESPONSE_CODE,
                                &easy_ptr->m_response_code
                            )
                        );
                        easy_ptr->parse();
                    }
                    catch (lgo::error_t e)
                    {
                        successful = false;
                        if (!handle_removed)
                        {
                            curl_multi_remove_handle(m_interface, message->easy_handle);
                        }
                        lgo::log_file(e.message() + "\n");
                    }
                    if (successful && easy_ptr->valid())
                    {
                        m_ready.emplace(message->easy_handle);
                    }
                    else
                    {
                        lgo::log_file
                        (
                            "A transfer failed in curl_multi_t::run:\n"
                            "  current_time:" + std::format("{:%T}", std::chrono::system_clock::now()) +
                            "  interface: " + std::format("{:016x}", reinterpret_cast<std::uint64_t>(message->easy_handle)) +
                            "  response_code: " + std::to_string(easy_ptr->m_response_code) +
                            "  request:\n" +
                            easy_ptr->m_read->m_payload +
                            "  response:\n" +
                            easy_ptr->m_write->m_payload
                        );
                        easy_ptr->resume();
                        api_curlm(curl_multi_add_handle(m_interface, easy_ptr->interface_get()));
                        ++m_running_count;
                        ++failed_count;
                    }
                    break;
                }
                default:
                {
                    lgo::log_file("curl_multi_t::run: An unknown message was received.");
                    break;
                }
            }
        }
        else if (message_count)
        {
            throw;
        }
        if (!message_count)
        {
            return failed_count;
        }
    }
}

auto curl_multi_t::running_count() -> int
{
    return m_running_count;
}

auto curl_multi_t::read_progress(CURL* key) -> std::uint64_t
{
    return m_added[key]->m_read->m_progress;
}

auto curl_multi_t::poll(int timeout_ms) -> void
{
    api_curlm(curl_multi_poll(m_interface, nullptr, 0, timeout_ms, nullptr));
    return;
}

auto curl_multi_t::ready_nonempty() -> bool
{
    return !m_ready.empty();
}
