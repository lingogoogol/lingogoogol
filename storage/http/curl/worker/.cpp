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
    std::mutex stop_mutex;
    std::mutex mutex;
    std::condition_variable changed;
    std::deque<std::unique_ptr<curl_easy_t>> pending;
    std::deque<std::unique_ptr<curl_easy_t>> completed;
    std::map<CURL*, std::uint64_t> progress;
    std::map<CURL *, std::shared_ptr<request_control_t>> controls;
    std::exception_ptr error;
    std::uint64_t failures{};
    std::atomic_bool stopping{};

    std::mutex sockets_mutex;
    std::map<SOCKET, std::shared_ptr<request_control_t>> sockets;
    struct socket_context_t
    {
        implementation_t* worker;
        std::shared_ptr<request_control_t> control;
    };
    // Stable contexts outlive every multi connection cache, including idle sockets.
    std::map<std::shared_ptr<request_control_t>, socket_context_t> socket_contexts;
    // Last: the thread must not observe partially constructed queue state.
    std::thread thread;

    implementation_t() : thread{ [this] { process(); } } {}

    static auto open_socket(void* data, curlsocktype, curl_sockaddr* address) noexcept -> curl_socket_t
    {
        auto& context{*static_cast<socket_context_t*>(data)};
        auto& self{*context.worker};
        try
        {
            std::unique_lock lock{ self.sockets_mutex };
            if (self.stopping) return CURL_SOCKET_BAD;
            SOCKET socket{context.control
                ? static_cast<SOCKET>(context.control->socket_open(address->family, address->socktype, address->protocol))
                : ::socket(address->family, address->socktype, address->protocol)};
            if (socket == INVALID_SOCKET) return CURL_SOCKET_BAD;
            try { self.sockets.emplace(socket, context.control); }
            catch (...)
            {
                if (context.control) context.control->socket_close(socket);
                else ::closesocket(socket);
                return CURL_SOCKET_BAD;
            }
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
        auto entry{self.sockets.find(socket)};
        if (entry == self.sockets.end()) return ::closesocket(socket);
        auto control{std::move(entry->second)};
        self.sockets.erase(entry);
        return control ? control->socket_close(socket) : ::closesocket(socket);
    }

    static auto transfer_progress(void* data, curl_off_t, curl_off_t, curl_off_t, curl_off_t) noexcept -> int
    {
        auto& easy{*static_cast<curl_easy_t*>(data)};
        return easy.cancelled() || easy.interrupted() ? 1 : 0;
    }

    auto configure(curl_easy_t& easy, bool enabled) -> void
    {
        auto* handle{ easy.interface_get() };
        auto& context{socket_contexts.try_emplace(easy.m_control, this, easy.m_control).first->second};
        api_curl(curl_easy_setopt(handle, CURLOPT_OPENSOCKETFUNCTION, enabled ? &open_socket : nullptr));
        api_curl(curl_easy_setopt(handle, CURLOPT_OPENSOCKETDATA, enabled ? &context : nullptr));
        api_curl(curl_easy_setopt(handle, CURLOPT_CLOSESOCKETFUNCTION, enabled ? &close_socket : nullptr));
        api_curl(curl_easy_setopt(handle, CURLOPT_CLOSESOCKETDATA, enabled ? this : nullptr));
        api_curl(curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, enabled ? &transfer_progress : nullptr));
        api_curl(curl_easy_setopt(handle, CURLOPT_XFERINFODATA, enabled ? &easy : nullptr));
        api_curl(curl_easy_setopt(handle, CURLOPT_NOPROGRESS, enabled ? 0L : 1L));
    }

    auto process() noexcept -> void
    {
        try
        {
            // Construct and destroy CURLM on this thread, including its cache of
            // idle connections. Socket callbacks stay alive through destruction.
            // A connection cache belongs to one task. Sharing CURLM across
            // controls would let socket shutdown abort another task (or HTTP/2
            // stream), and could spend its independent network retry budget.
            // Pause shuts down all task sockets, even idle ones. A generation
            // change retires that cache even if Continue was clicked before
            // this worker observed the paused flag.
            using group_key_t = std::pair<std::shared_ptr<request_control_t>, unsigned>;
            std::map<group_key_t, std::unique_ptr<curl_multi_t>> multis;
            std::map<CURL*, curl_multi_t*> active;
            std::map<group_key_t, std::chrono::steady_clock::time_point> last_used;
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
                    const group_key_t group{easy->m_control, easy->pause_generation()};
                    auto& multi{multis[group]};
                    if (!multi) multi = std::make_unique<curl_multi_t>();
                    last_used[group] = std::chrono::steady_clock::now();
                    multi->add(std::move(easy));
                    active.emplace(key, multi.get());
                }

                // No queue mutex is held across curl, TLS waits, parsing or retry.
                std::uint64_t failed{};
                std::map<CURL*, std::uint64_t> snapshot;
                std::deque<std::unique_ptr<curl_easy_t>> results;
                for (auto& [group, multi] : multis)
                {
                    failed += multi->run(&stopping);
                    if (stopping) break;
                    while (multi->ready_nonempty())
                    {
                        auto easy{multi->ready_get_unique()};
                        active.erase(easy->interface_get());
                        configure(*easy, false);
                        results.push_back(std::move(easy));
                    }
                }
                if (stopping) break;
                for (auto [key, multi] : active) snapshot.emplace(key, multi->read_progress(key));
                // Paused task caches must release their idle connections too.
                std::erase_if(multis, [&](const auto& entry)
                {
                    const bool busy{std::ranges::any_of(active, [&](const auto& request)
                        { return request.second == entry.second.get(); })};
                    if (busy) last_used[entry.first] = std::chrono::steady_clock::now();
                    const bool expired{std::chrono::steady_clock::now() - last_used.at(entry.first) > std::chrono::seconds{30}};
                    const auto& [control, generation]{entry.first};
                    const bool retired{control && (control->cancelled || control->user_paused ||
                                                   control->pause_epoch != generation)};
                    const bool remove{!busy && (expired || retired)};
                    if (remove) last_used.erase(entry.first);
                    return remove;
                });
                std::erase_if(socket_contexts, [&](const auto& entry)
                {
                    return !std::ranges::any_of(multis,
                        [&](const auto& group) { return group.first.first == entry.first; });
                });
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
            for (const auto& [socket, control] : sockets) ::shutdown(socket, SD_BOTH);
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
    // Latch on the submitting thread. A Pause/Continue while queued must still
    // abort this submission, rather than silently adopting a shutdown cache.
    easy->acknowledge_pause();
    m_impl->controls[easy->interface_get()] = easy->m_control;
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
    if (m_impl->completed.empty()) throw std::logic_error{"No completed transfer is available."};
    auto result{std::move(m_impl->completed.front())};
    m_impl->completed.pop_front();
    m_impl->progress.erase(result->interface_get());
    m_impl->controls.erase(result->interface_get());
    return result;
}

auto curl_multi_worker_t::stop() -> void
{
    std::lock_guard stop_lock{m_impl->stop_mutex};
    m_impl->request_stop();
    if (m_impl->thread.joinable()) m_impl->thread.join();
    std::unique_lock lock{ m_impl->mutex };
    m_impl->pending.clear();
    m_impl->completed.clear();
    m_impl->progress.clear();
}

auto curl_multi_worker_t::interrupt() -> void
{
    std::lock_guard lock{m_impl->sockets_mutex};
    // Explicit global interruption (for whole-account clearing). Ordinary
    // pause/cancel shuts down only the sockets registered with that control.
    for (const auto& [socket, control] : m_impl->sockets)
        ::shutdown(socket, SD_BOTH);
    m_impl->changed.notify_all();
}
auto curl_multi_worker_t::pending(const std::shared_ptr<request_control_t> &control) const -> bool
{
    std::lock_guard lock{m_impl->mutex};
    return std::ranges::any_of(m_impl->controls, [&](const auto &entry) { return entry.second == control; });
}
