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
            if (!m_ready.contains(added.first) && !m_waiting.contains(added.first))
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

auto curl_multi_t::add(std::unique_ptr<curl_easy_t> easy) -> void
{
    if (!easy || !easy->nonempty()) throw std::invalid_argument{ "Cannot submit an empty curl request." };
    CURL* key{ easy->interface_get() };
    easy->m_suspended = false;
    if (m_added.contains(key)) throw std::logic_error{ "Curl request is already submitted." };
    curl_easy_multi_access_t::write_get(*easy) = std::make_unique<curl_easy_t::write_t>();
    api_curl(curl_easy_setopt(key, CURLOPT_WRITEDATA,
        static_cast<void*>(curl_easy_multi_access_t::write_get(*easy).get())));
    auto [entry, inserted]{ m_added.emplace(key, std::move(easy)) };
    try { m_waiting.emplace(key); }
    catch (...) { m_added.erase(entry); throw; }
}

auto curl_multi_t::run(const std::atomic_bool* cancelled) -> std::uint64_t
{
    // Abort on the curl owner thread; never invoke completion callbacks for a
    // partial response or charge an intentional pause to the retry budget.
    for (auto entry{m_active.begin()}; entry != m_active.end();)
    {
        auto& easy{*m_added.at(*entry)};
        if (easy.cancelled() || easy.interrupted())
        {
            api_curlm(curl_multi_remove_handle(m_interface, *entry));
            easy.m_suspended = !easy.cancelled();
            if (easy.m_suspended) easy.curl_easy_t::resume();
            m_ready.emplace(*entry);
            entry = m_active.erase(entry);
        }
        else ++entry;
    }
    // Waiting for credentials must not stall other transfers in this multi.
    for (auto entry{ m_waiting.begin() }; entry != m_waiting.end();)
    {
        if (cancelled && cancelled->load()) return 0;
        auto& easy{ *m_added.at(*entry) };
        if (easy.cancelled() || easy.interrupted())
        {
            easy.m_suspended = !easy.cancelled();
            m_ready.emplace(*entry);
            entry = m_waiting.erase(entry);
            continue;
        }
        if (!easy.runnable())
        {
            ++entry;
            continue;
        }
        try
        {
            if (easy.replay())
            {
                m_ready.emplace(*entry);
                entry = m_waiting.erase(entry);
                continue;
            }
            if (!easy.prepare())
            {
                ++entry;
                continue;
            }
        }
        catch (const std::exception &error)
        {
            if (!easy.m_control)
                throw;
            easy.m_control->fail(error.what());
            ++entry;
            continue;
        }
        curl_easy_multi_access_t::response_code_get(easy) = 0;
        api_curlm(curl_multi_add_handle(m_interface, *entry));
        m_active.emplace(*entry);
        entry = m_waiting.erase(entry);
    }
    api_curlm(curl_multi_perform(m_interface, &m_running_count));
    int message_count{};
    std::uint64_t failed_count{ 0 };
    while (true)
    {
        // Socket shutdown may have interrupted Schannel inside perform(). Do not
        // parse, regenerate or retry requests while the owner is stopping.
        if (cancelled && cancelled->load()) return failed_count;
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
                        // A task-owned socket shutdown may have woken Schannel.
                        // Detach before interpreting the intentional abort.
                        if (easy_ptr->cancelled() || (message->data.result != CURLE_OK && easy_ptr->interrupted()))
                        {
                            curl_multi_remove_handle(m_interface, message->easy_handle);
                            handle_removed = true;
                            easy_ptr->m_suspended = !easy_ptr->cancelled();
                            if (easy_ptr->m_suspended) easy_ptr->curl_easy_t::resume();
                            m_active.erase(message->easy_handle);
                            m_ready.emplace(message->easy_handle);
                            break;
                        }
                        api_curl(message->data.result);
                        curl_multi_remove_handle(m_interface, message->easy_handle);
                        handle_removed = true;
                        api_curl
                        (
                            curl_easy_getinfo
                            (
                                message->easy_handle,
                                CURLINFO_RESPONSE_CODE,
                                &curl_easy_multi_access_t::response_code_get(*easy_ptr)
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
                    if (cancelled && cancelled->load()) return failed_count;
                    m_active.erase(message->easy_handle);
                    if (easy_ptr->cancelled())
                    {
                        m_ready.emplace(message->easy_handle);
                        break;
                    }
                    if (successful && easy_ptr->valid())
                    {
                        easy_ptr->checkpoint();
                        m_ready.emplace(message->easy_handle);
                    }
                    else if (easy_ptr->interrupted())
                    {
                        easy_ptr->m_suspended = true;
                        easy_ptr->curl_easy_t::resume();
                        m_ready.emplace(message->easy_handle);
                    }
                    else
                    {
                        lgo::log_file
                        (
                            "A transfer failed in curl_multi_t::run:\n"
                            "  current_time:" + std::format("{:%T}", std::chrono::system_clock::now()) +
                            "  interface: " + std::format("{:016x}", reinterpret_cast<std::uint64_t>(message->easy_handle)) +
                            "  response_code: " + std::to_string(curl_easy_multi_access_t::response_code_get(*easy_ptr)) + "\n"
                        );
                        const auto reason = successful ? easy_ptr->failure_reason() : "Network connection failed.";
                        lgo::log_file(reason + "\n");
                        easy_ptr->failed(reason);
                        m_waiting.emplace(message->easy_handle);
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
    return m_running_count + static_cast<int>(m_waiting.size());
}

auto curl_multi_t::read_progress(CURL* key) -> std::uint64_t
{
    auto found{ m_added.find(key) };
    return found == m_added.end() ? 0 : curl_easy_multi_access_t::read_get(*found->second)->m_progress;
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
