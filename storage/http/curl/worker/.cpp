module;

// This project targets Windows, including when cross compiled from Linux.
// Winsock shutdown can wake Schannel's blocking select without closing/reusing
// a socket underneath curl; only the worker performs closesocket and curl APIs.
#include <winsock2.h>

module storage.http.curl.worker;

import std;

import external.curl;
import storage.http.curl.curl;
import storage.http.curl.easy;
import storage.http.curl.multi;

struct curl_multi_worker_t::implementation_t
{
    std::mutex mutex;
    std::condition_variable changed;
    std::deque<std::unique_ptr<curl_easy_t>> pending;
    std::deque<std::unique_ptr<curl_easy_t>> completed;
    std::map<CURL*, std::uint64_t> progress;
    std::exception_ptr error;
    std::uint64_t failures{};
    std::atomic_bool stopping{};

    std::mutex sockets_mutex;
    std::set<SOCKET> sockets;
    // Last: the thread must not observe partially constructed queue state.
    std::thread thread;

    implementation_t() : thread{ [this] { process(); } } {}

    static auto open_socket(void* data, curlsocktype, curl_sockaddr* address) noexcept -> curl_socket_t
    {
        auto& self{ *static_cast<implementation_t*>(data) };
        try
        {
            std::unique_lock lock{ self.sockets_mutex };
            if (self.stopping) return CURL_SOCKET_BAD;
            SOCKET socket{ ::socket(address->family, address->socktype, address->protocol) };
            if (socket == INVALID_SOCKET) return CURL_SOCKET_BAD;
            try { self.sockets.emplace(socket); }
            catch (...) { ::closesocket(socket); return CURL_SOCKET_BAD; }
            return socket;
        }
        catch (...) { return CURL_SOCKET_BAD; }
    }

    static auto close_socket(void* data, curl_socket_t socket) noexcept -> int
    {
        auto& self{ *static_cast<implementation_t*>(data) };
        // Keep unregister, close and cancellation serialized to avoid shutting
        // down an unrelated socket that reused the same numeric handle.
        std::unique_lock lock{ self.sockets_mutex };
        self.sockets.erase(socket);
        return ::closesocket(socket);
    }

    static auto transfer_progress(void* data, curl_off_t, curl_off_t, curl_off_t, curl_off_t) noexcept -> int
    {
        return static_cast<implementation_t*>(data)->stopping.load() ? 1 : 0;
    }

    auto configure(curl_easy_t& easy, bool enabled) -> void
    {
        auto* handle{ easy.interface_get() };
        api_curl(curl_easy_setopt(handle, CURLOPT_OPENSOCKETFUNCTION, enabled ? &open_socket : nullptr));
        api_curl(curl_easy_setopt(handle, CURLOPT_OPENSOCKETDATA, enabled ? this : nullptr));
        api_curl(curl_easy_setopt(handle, CURLOPT_CLOSESOCKETFUNCTION, enabled ? &close_socket : nullptr));
        api_curl(curl_easy_setopt(handle, CURLOPT_CLOSESOCKETDATA, enabled ? this : nullptr));
        api_curl(curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, enabled ? &transfer_progress : nullptr));
        api_curl(curl_easy_setopt(handle, CURLOPT_XFERINFODATA, enabled ? this : nullptr));
        api_curl(curl_easy_setopt(handle, CURLOPT_NOPROGRESS, enabled ? 0L : 1L));
    }

    auto process() noexcept -> void
    {
        try
        {
            // Construct and destroy CURLM on this thread, including its cache of
            // idle connections. Socket callbacks stay alive through destruction.
            curl_multi_t multi;
            std::set<CURL*> active;
            while (!stopping)
            {
                std::deque<std::unique_ptr<curl_easy_t>> additions;
                {
                    std::unique_lock lock{ mutex };
                    if (active.empty()) changed.wait(lock, [this] { return stopping || !pending.empty(); });
                    additions.swap(pending);
                }
                if (stopping) break;
                for (auto& easy : additions)
                {
                    if (stopping) break;
                    configure(*easy, true);
                    auto* key{ easy->interface_get() };
                    multi.add(std::move(easy));
                    active.emplace(key);
                }

                // No queue mutex is held across curl, TLS waits, parsing or retry.
                auto failed{ multi.run(&stopping) };
                if (stopping) break;
                std::map<CURL*, std::uint64_t> snapshot;
                for (auto* key : active) snapshot.emplace(key, multi.read_progress(key));
                std::deque<std::unique_ptr<curl_easy_t>> results;
                while (multi.ready_nonempty())
                {
                    auto easy{ multi.ready_get_unique() };
                    active.erase(easy->interface_get());
                    // The GUI can destroy/reuse this handle as soon as it is
                    // published. Remove pointers into our worker before handing it off.
                    configure(*easy, false);
                    results.push_back(std::move(easy));
                }
                {
                    std::unique_lock lock{ mutex };
                    failures += failed;
                    for (auto [key, bytes] : snapshot) progress[key] = bytes;
                    while (!results.empty())
                    {
                        completed.push_back(std::move(results.front()));
                        results.pop_front();
                    }
                    // Avoid busy spinning while connections are waiting, but wake
                    // promptly for new requests and shutdown. GUI polling never waits.
                    if (!active.empty()) changed.wait_for(lock, std::chrono::milliseconds{ 5 },
                        [this] { return stopping || !pending.empty(); });
                }
            }
        }
        catch (...)
        {
            if (!stopping)
            {
                std::unique_lock lock{ mutex };
                error = std::current_exception();
            }
        }
    }

    auto request_stop() -> void
    {
        {
            // Serialize with the idle wait predicate so shutdown cannot lose its wakeup.
            std::unique_lock lock{ mutex };
            stopping = true;
        }
        {
            std::unique_lock lock{ sockets_mutex };
            for (auto socket : sockets) ::shutdown(socket, SD_BOTH);
        }
        changed.notify_all();
    }
};

curl_multi_worker_t::curl_multi_worker_t() : m_impl{ std::make_unique<implementation_t>() } {}

curl_multi_worker_t::~curl_multi_worker_t()
{
    stop();
}

auto curl_multi_worker_t::submit(std::unique_ptr<curl_easy_t> easy) -> void
{
    if (!easy || !easy->nonempty()) throw std::invalid_argument{ "Cannot submit an empty curl request." };
    std::unique_lock lock{ m_impl->mutex };
    if (m_impl->error) std::rethrow_exception(m_impl->error);
    if (m_impl->stopping) throw std::logic_error{ "Cannot submit to a stopped transfer worker." };
    if (m_impl->progress.contains(easy->interface_get()))
        throw std::logic_error{ "Curl request is already submitted." };
    m_impl->progress.emplace(easy->interface_get(), 0);
    m_impl->pending.push_back(std::move(easy));
    lock.unlock();
    m_impl->changed.notify_one();
}

auto curl_multi_worker_t::run() -> std::uint64_t
{
    std::unique_lock lock{ m_impl->mutex };
    if (m_impl->error) std::rethrow_exception(m_impl->error);
    return std::exchange(m_impl->failures, 0);
}

auto curl_multi_worker_t::read_progress(CURL* key) const -> std::uint64_t
{
    std::unique_lock lock{ m_impl->mutex };
    auto found{ m_impl->progress.find(key) };
    return found == m_impl->progress.end() ? 0 : found->second;
}

auto curl_multi_worker_t::ready_nonempty() const -> bool
{
    std::unique_lock lock{ m_impl->mutex };
    return !m_impl->completed.empty();
}

auto curl_multi_worker_t::ready_get_unique() -> std::unique_ptr<curl_easy_t>
{
    std::unique_lock lock{ m_impl->mutex };
    if (m_impl->completed.empty()) throw std::logic_error{ "No completed transfer is available." };
    auto result{ std::move(m_impl->completed.front()) };
    m_impl->completed.pop_front();
    m_impl->progress.erase(result->interface_get());
    return result;
}

auto curl_multi_worker_t::stop() -> void
{
    m_impl->request_stop();
    if (m_impl->thread.joinable()) m_impl->thread.join();
    std::unique_lock lock{ m_impl->mutex };
    m_impl->pending.clear();
    m_impl->completed.clear();
    m_impl->progress.clear();
}
