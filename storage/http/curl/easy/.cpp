module;

#include <stdio.h>
#include <nlohmann/json.hpp>

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
    configure_transport();
}

auto curl_easy_t::configure_transport() -> void
{
    if (!m_interface) throw std::runtime_error{"Cannot create curl handle."};
    curl_easy_reset(m_interface);
	api_curl(curl_easy_setopt(m_interface, CURLOPT_READFUNCTION, &curl_easy_t::read_callback));
	api_curl(curl_easy_setopt(m_interface, CURLOPT_READDATA, static_cast<void*>(m_read.get())));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_SEEKFUNCTION, &curl_easy_t::seek_callback));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_SEEKDATA, static_cast<void*>(m_read.get())));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_WRITEFUNCTION, &curl_easy_t::write_callback));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_WRITEDATA, static_cast<void*>(m_write.get())));
    // Verbose headers expose Bearer credentials on stderr even in Release.
    api_curl(curl_easy_setopt(m_interface, CURLOPT_VERBOSE, 0L));
    #ifdef LGO_DEBUG_CURL
    api_curl(curl_easy_setopt(m_interface, CURLOPT_DEBUGDATA, this));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_DEBUGFUNCTION, &debug_callback));
    #endif
    api_curl(curl_easy_setopt(m_interface, CURLOPT_TIMEOUT, 300L));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_CONNECTTIMEOUT, 100L));
    return;
}

curl_easy_t::curl_easy_t(curl_easy_t &&in)
    : m_account_context{std::move(in.m_account_context)}, m_control{std::move(in.m_control)},
      m_journal{std::move(in.m_journal)}, m_suspended{in.m_suspended}, m_sync_multi{std::move(in.m_sync_multi)},
      m_sync_pause_epoch{in.m_sync_pause_epoch}, m_verb{std::move(in.m_verb)},
      m_cache_key{std::move(in.m_cache_key)}, m_failures{in.m_failures}, m_epoch{in.m_epoch}, m_pause_epoch{in.m_pause_epoch},
      m_retry_at{in.m_retry_at}, m_interface{in.m_interface}, m_host{std::move(in.m_host)},
      m_path{std::move(in.m_path)}, m_query{std::move(in.m_query)}, m_header{std::move(in.m_header)},
      m_read{std::move(in.m_read)}, m_write{std::move(in.m_write)}, m_response_code{in.m_response_code},
      m_header_curl{in.m_header_curl}
{
    in.m_interface = nullptr;
    in.m_header_curl = nullptr;
    return;
}

auto curl_easy_t::operator=(curl_easy_t&& in) -> curl_easy_t&
{
    if (this == &in) return *this;
    free();
    m_account_context = std::move(in.m_account_context);
    m_control = std::move(in.m_control);
    m_journal = std::move(in.m_journal);
    m_sync_multi = std::move(in.m_sync_multi);
    m_sync_pause_epoch = in.m_sync_pause_epoch;
    m_verb = std::move(in.m_verb);
    m_cache_key = std::move(in.m_cache_key);
    m_failures = in.m_failures;
    m_epoch = in.m_epoch;
    m_pause_epoch = in.m_pause_epoch;
    m_suspended = in.m_suspended;
    m_retry_at = in.m_retry_at;
    m_interface = in.m_interface;
    m_path = std::move(in.m_path);
    m_host = std::move(in.m_host);
    m_query = std::move(in.m_query);
    m_header = std::move(in.m_header);
    m_read = std::move(in.m_read);
    m_write = std::move(in.m_write);
    m_response_code = in.m_response_code;
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
    // Socket callbacks still refer to m_control until the connection cache dies.
    m_sync_multi.reset();
    //Passing in a nullptr makes curl_easy_cleanup return immediately with no action.
    curl_easy_cleanup(std::exchange(m_interface, nullptr));
    curl_slist_free_all(std::exchange(m_header_curl, nullptr));
    return;
}

auto curl_easy_t::release() -> void
{
    m_interface = nullptr;
    return;
}

auto curl_easy_t::take_transport() -> CURL*
{
    m_sync_multi.reset();
    return std::exchange(m_interface, nullptr);
}

auto curl_easy_t::bind_transport(CURL* handle) -> void
{
    if (m_interface || !handle) throw std::logic_error{"Invalid curl transport handoff."};
    m_interface = handle;
    configure_transport();
    // Reconstruct only transport options. Calling the virtual method builders
    // here would reset Google authentication retries and batch request state.
    curl_easy_t::method();
    if (m_verb == "POST")
        api_curl(curl_easy_setopt(handle, CURLOPT_POST, 1L));
    else if (m_verb == "PATCH")
    {
        api_curl(curl_easy_setopt(handle, CURLOPT_UPLOAD, 1L));
        api_curl(curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "PATCH"));
    }
    else if (m_verb == "DELETE")
        api_curl(curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "DELETE"));
    m_suspended = false;
    acknowledge_pause();
}

auto curl_easy_t::interrupted() const -> bool
{
    return m_control && (m_control->user_paused || m_control->pause_epoch != m_pause_epoch);
}

auto curl_easy_t::acknowledge_pause() -> void
{
    m_pause_epoch = m_control ? m_control->pause_epoch.load() : 0;
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
    m_cache_key.clear();
    m_verb.clear();
    m_failures = 0;
    m_suspended = false;
    m_retry_at = {};
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
    m_verb = "GET";
    m_cache_key.clear();
    m_read->m_payload.clear();
    method();
	api_curl(curl_easy_setopt(m_interface, CURLOPT_CUSTOMREQUEST, nullptr));
	api_curl(curl_easy_setopt(m_interface, CURLOPT_UPLOAD, 0L));
    //When setting CURLOPT_HTTPGET to 1, libcurl automatically sets CURLOPT_NOBODY to 0 and CURLOPT_UPLOAD to 0.
	api_curl(curl_easy_setopt(m_interface, CURLOPT_HTTPGET, 1L));
    return;
}

auto curl_easy_t::method_post(std::string payload) -> void
{
    m_verb = "POST";
    m_cache_key.clear();
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
    m_verb = "PATCH";
    m_cache_key.clear();
    method();
	api_curl(curl_easy_setopt(m_interface, CURLOPT_CUSTOMREQUEST, "PATCH"));
	api_curl(curl_easy_setopt(m_interface, CURLOPT_UPLOAD, 1L));
    m_read->m_progress = 0;
    m_read->m_payload = payload;
    return;
}

auto curl_easy_t::run() -> void
{
    while (true)
	{
		run_try();
        if (valid())
        {
            break;
        }
        lgo::log_file
        (
            "A transfer failed in curl_easy_t::run:"
            "  current_time:" + std::format("{:%T}", std::chrono::system_clock::now()) +
            "  interface: " + std::format("{:016x}", reinterpret_cast<std::uint64_t>(m_interface)) +
            // OAuth bodies contain signed assertions or access tokens. Keep
            // failure diagnostics, but never dump arbitrary request/response bodies.
            "  response_code: " + std::to_string(m_response_code) + "\n"
        );
        //Rewind the request body before retrying a failed PATCH/POST. Otherwise
        //folder publication retries send an empty body after the first attempt.
        const auto reason = failure_reason();
        lgo::log_file(reason + "\n");
        failed(reason);
        }
	return;
}

auto curl_easy_t::run_try() -> void
{
    while (true)
    {
        acknowledge_pause();
        while (!runnable())
        {
            if (cancelled())
                throw std::runtime_error{"Task cancelled."};
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
        }
        if (replay())
            return;
        bool prepared{};
        while (!prepared)
        {
            if (cancelled())
                throw std::runtime_error{"Task cancelled."};
            if (!runnable())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds{5});
                continue;
            }
            try
            {
                prepared = prepare();
            }
            catch (const std::exception &error)
            {
                if (!m_control)
                    throw;
                m_control->fail(error.what());
            }
            if (!prepared)
                std::this_thread::sleep_for(std::chrono::milliseconds{5});
        }
        if (interrupted()) continue;
        m_response_code = 0;
        m_write = std::make_unique<write_t>();
        api_curl(curl_easy_setopt(m_interface, CURLOPT_WRITEDATA, static_cast<void*>(m_write.get())));
        try
        {
            if (!m_control)
                api_curl(curl_easy_perform(m_interface));
            else
            {
                // Preparation and publication run on their own threads. Drive a
                // local multi so user pause can abort in-flight chunk/preparation I/O;
                // sleeping in a curl callback would prevent cancellation/polling.
                // A quick Pause/Continue also invalidates cached connections
                // after a completed request; never reuse a shutdown socket.
                if (m_sync_multi && m_sync_pause_epoch != m_pause_epoch)
                    m_sync_multi.reset();
                if (!m_sync_multi)
                {
                    m_sync_multi.reset(curl_multi_init());
                    m_sync_pause_epoch = m_pause_epoch;
                }
                auto* multi{m_sync_multi.get()};
                if (!multi)
                    throw std::runtime_error{"Cannot create controlled curl multi."};
                auto checked = [](CURLMcode result)
                {
                    if (result != CURLM_OK)
                        throw lgo::error_t{curl_multi_strerror(result)};
                };
                checked(curl_multi_add_handle(multi, m_interface));
                bool aborted{};
                bool finished{};
                CURLcode result{CURLE_OK};
                try
                {
                    int running{1};
                    while (running && !cancelled() && !interrupted())
                    {
                        checked(curl_multi_perform(multi, &running));
                        if (running)
                            checked(curl_multi_poll(multi, nullptr, 0, 5, nullptr));
                    }
                    aborted = interrupted();
                    int remaining{};
                    while (auto* message{curl_multi_info_read(multi, &remaining)})
                        if (message->msg == CURLMSG_DONE)
                        {
                            result = message->data.result;
                            finished = true;
                        }
                }
                catch (...)
                {
                    curl_multi_remove_handle(multi, m_interface);
                    throw;
                }
                checked(curl_multi_remove_handle(multi, m_interface));
                if (aborted && !(finished && result == CURLE_OK))
                {
                    // Intentional abort is not a network/authentication failure.
                    curl_easy_t::resume();
                    m_response_code = 0;
                    // No active transfer or idle connection is retained while paused.
                    m_sync_multi.reset();
                    acknowledge_pause();
                    continue;
                }
                if (cancelled())
                    throw std::runtime_error{"Task cancelled."};
                api_curl(result);
            }
		    api_curl(curl_easy_getinfo(m_interface, CURLINFO_RESPONSE_CODE, &m_response_code));
        }
        catch (lgo::error_t error)
        {
            lgo::log_file(std::string{ error.what() } + "\n");
            //A connection failure can also leave a partially consumed request body.
            failed("Network connection failed.");
            continue;
        }
        break;
    }
    parse();
    if (valid())
        checkpoint();
    return;
}

auto curl_easy_t::prepare() -> bool { return true; }

auto curl_easy_t::parse() -> void {}

auto curl_easy_t::valid() -> bool
{
    return m_response_code >= 200 && m_response_code < 300;
}

auto curl_easy_t::failure_reason() const -> std::string
{
    return "HTTP request failed (status " + std::to_string(m_response_code) + ").";
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

auto curl_easy_t::method_delete() -> void
{
    method_get();
    m_verb = "DELETE";
    api_curl(curl_easy_setopt(m_interface, CURLOPT_CUSTOMREQUEST, "DELETE"));
}
auto curl_easy_t::control_set(std::shared_ptr<request_control_t> control) -> void
{
    // Close cached sockets before releasing the callback context that opened
    // them. An easy may be reused with another task/account on its next request.
    if (control != m_control)
        m_sync_multi.reset();
    m_control = std::move(control);
    m_epoch = m_control ? m_control->epoch.load() : 0;
    acknowledge_pause();
    if (!m_control)
    {
        api_curl(curl_easy_setopt(m_interface, CURLOPT_OPENSOCKETFUNCTION, nullptr));
        api_curl(curl_easy_setopt(m_interface, CURLOPT_CLOSESOCKETFUNCTION, nullptr));
        api_curl(curl_easy_setopt(m_interface, CURLOPT_NOPROGRESS, 1L));
        return;
    }
    // Synchronous preparation/listing workers need the same Schannel wake-up
    // as the main multi worker, otherwise their future can delay shutdown 300s.
    api_curl(curl_easy_setopt(
        m_interface, CURLOPT_OPENSOCKETFUNCTION,
        +[](void *context, curlsocktype, curl_sockaddr *address) -> curl_socket_t
        {
            return static_cast<curl_socket_t>(static_cast<request_control_t *>(context)->socket_open(
                address->family, address->socktype, address->protocol));
        }));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_OPENSOCKETDATA, m_control.get()));
    api_curl(curl_easy_setopt(
        m_interface, CURLOPT_CLOSESOCKETFUNCTION, +[](void *context, curl_socket_t socket) -> int
        { return static_cast<request_control_t *>(context)->socket_close(socket); }));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_CLOSESOCKETDATA, m_control.get()));
    api_curl(curl_easy_setopt(
        m_interface, CURLOPT_XFERINFOFUNCTION,
        +[](void *context, curl_off_t, curl_off_t, curl_off_t, curl_off_t) -> int
        { return context && static_cast<request_control_t *>(context)->cancelled ? 1 : 0; }));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_XFERINFODATA, m_control.get()));
    api_curl(curl_easy_setopt(m_interface, CURLOPT_NOPROGRESS, m_control ? 0L : 1L));
}
auto curl_easy_t::runnable() -> bool
{
    if (m_control)
    {
        if (m_epoch != m_control->epoch)
        {
            m_epoch = m_control->epoch;
            m_failures = 0;
            m_retry_at = {};
            manual_retry();
        }
        if (m_control->cancelled || m_control->paused || m_control->user_paused)
            return false;
    }
    return std::chrono::steady_clock::now() >= m_retry_at;
}

auto curl_easy_t::manual_retry() -> void
{
    resume();
}
auto curl_easy_t::authentication_failure() const -> bool
{
    return m_response_code == 401;
}
auto curl_easy_t::failed(std::string reason) -> void
{
    const bool auth{authentication_failure()};
    try
    {
        resume();
    }
    catch (const std::exception &error)
    {
        if (!m_control)
            throw;
        m_control->fail(error.what());
        return;
    }
    if (m_control && !auth)
    {
        const int limit{m_control->retry_limit};
        if (limit >= 0 && m_failures >= static_cast<unsigned>(limit))
            m_control->fail(std::move(reason));
        else
        {
            // Unlimited retries still need backoff. Incrementing only inside
            // the finite-limit condition kept the default (-1) at one second.
            if (m_failures < std::numeric_limits<unsigned>::max()) ++m_failures;
            m_retry_at = std::chrono::steady_clock::now() +
                std::chrono::seconds{std::min(29u, m_failures) + 1};
        }
    }
}
auto curl_easy_t::replay() -> bool
{
    if (!m_journal)
        return false;
    if (m_cache_key.empty())
    {
        std::string request{m_verb + " " + m_host + m_path + "\n"};
        for (const auto &[name, value] : m_query)
            request += name + "=" + value + "\n";
        request += m_read->m_payload;
        m_cache_key = storage_digest(request);
    }
    if (auto saved{m_journal->response(m_cache_key)})
    {
        restore_response(*saved);
        return true;
    }
    return false;
}
auto curl_easy_t::checkpoint() -> void
{
    if (m_journal && !m_cache_key.empty())
        m_journal->response(m_cache_key, saved_response());
}
auto curl_easy_t::saved_response() -> std::string
{
    return nlohmann::json{{"status", m_response_code}, {"body", m_write->m_payload}}.dump();
}
auto curl_easy_t::restore_response(std::string_view value) -> void
{
    const auto saved = nlohmann::json::parse(value);
    m_response_code = saved.at("status").get<long>();
    m_write->m_payload = saved.at("body").get<std::string>();
    parse();
    if (!valid())
        throw std::runtime_error{"Invalid saved HTTP response."};
}
