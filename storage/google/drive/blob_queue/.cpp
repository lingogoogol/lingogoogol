module;

#include <nlohmann/json.hpp>

module storage.google.drive.blob_queue;

import std;

import lgo.io.file;

import storage.http.curl;
import storage.google.account;
import storage.google.batch;
import storage.google.drive.blob;

auto blob_queue_t::process() -> void
{
    using namespace std::chrono_literals;

    auto &multi{m_worker};
    // A array of easy handle that is currently not used.
    constexpr std::size_t batch_size{100}, batch_limit{64}, reserve_target{20000};
    std::vector<google_batch_t> batch_idle_arr(batch_limit);
    std::size_t in_flight{};
    for (auto& batch : batch_idle_arr)
    {
        batch = google_batch_t{ curl_easy_t{} };
        batch.control_set(m_control);
        batch.m_account_context = std::shared_ptr<google_account_t>{m_account, [](google_account_t *) {}};
    }

    std::chrono::steady_clock::time_point last_cycle_time{};
    while (!*m_close && !m_stopped)
    {
        std::this_thread::sleep_until(last_cycle_time + 0.1s);
        if (*m_close || m_stopped)
            return;
        std::chrono::high_resolution_clock::time_point now{ std::chrono::high_resolution_clock::now() };
        last_cycle_time = now;
        std::unique_lock cycle_lock{ m_mutex };
        m_cycle_time.push_back(now);
        while (m_cycle_time.front() <= now - 1s)
        {
            m_cycle_time.pop_front();
        }
        cycle_lock.unlock();

        multi.run();
        while (true)
        {
            if (!multi.ready_nonempty())
            {
                break;
            }
            auto response{multi.ready_get_unique()};
            google_batch_t batch{std::move(dynamic_cast<google_batch_t &>(*response))};
            --in_flight;
            std::unique_lock lock{ m_mutex };
            if (m_stopped)
                return;
            if (!m_cleared)
            {
                std::vector<std::string> ids;
                for (std::uint64_t i{ 0 }; i < 100; ++i)
                    ids.push_back(nlohmann::json::parse(batch.response_get(i).body_get())["id"]);
                append_locked(ids);
                m_condition_variable.notify_all();
            }
            batch.clear();
            m_cleared = false;
            batch_idle_arr.push_back(std::move(batch));
        }

        std::unique_lock lock{ m_mutex };
        // Pending creates already cover part of the reserve. Counting only
        // completed IDs would keep replenishing the same gap while requests
        // are slow or retrying and overshoot the target by thousands of IDs.
        const auto projected=m_id.size()+in_flight*batch_size;
        if (m_clearing || projected>=reserve_target)
        {
            continue;
        }
        const auto needed=(reserve_target-projected+batch_size-1)/batch_size;
        const auto submit_count=std::min({batch_limit,batch_idle_arr.size(),needed});
        lock.unlock();
        for (std::size_t submitted{};submitted<submit_count;++submitted)
        {
            google_batch_t batch{ std::move(batch_idle_arr.back()) };
            batch_idle_arr.pop_back();

            batch.path_set("/drive/v3");
            for (std::size_t i{};i<batch_size;++i)
            {
                blob_upload(batch, "", "");
            }
            batch.method_post();
            multi.add(std::move(batch));
            ++in_flight;
            m_planned_creations.fetch_add(batch_size,std::memory_order_relaxed);
        }
    }
    return;
}

blob_queue_t::blob_queue_t(std::filesystem::path path, google_account_t* account, const std::atomic_bool* close)
: m_path{ path }, m_account{ account }, m_close{ close }
{
    recover_locked();
    m_thread = std::thread{ [this]
    {
        try { process(); }
        catch (...)
        {
            {
                std::unique_lock lock{ m_mutex };
                if (!m_stopped) m_error = std::current_exception();
            }
            m_condition_variable.notify_all();
        }
    } };
    return;
}

auto blob_queue_t::stop() -> void
{
    std::lock_guard stop_lock{m_stop_mutex};
    // Publish cancellation under the same mutex as get()'s predicate to avoid lost wakeups.
    {
        std::unique_lock lock{ m_mutex };
        m_stopped = true;
    }
    m_condition_variable.notify_all();
    m_worker.stop();
    if (m_thread.joinable())
        m_thread.join();
}

blob_queue_t::~blob_queue_t()
{
    stop();
    // Every mutation is already durable. Writing again here could overwrite a
    // replacement queue created for the same account after a completed clear.
    return;
}

auto blob_queue_t::get() -> std::string
{
    std::unique_lock lock{m_mutex};
    if (m_needs_recovery) recover_locked();
    while (!m_stopped && !m_error && !m_control->paused && (m_clearing || m_id.size() <= 1))
        m_condition_variable.wait_for(lock, std::chrono::milliseconds{50});
    if (m_stopped)
        throw std::runtime_error{"Blob queue stopped."};
    if (m_error) std::rethrow_exception(m_error);
    if (m_control->paused)
        throw std::runtime_error{m_control->reason()};
    auto result{m_id.back()};
    trim_locked(1);
    return result;
}

auto blob_queue_t::get(std::uint64_t count, const std::atomic_bool *cancelled) -> std::vector<std::string>
{
    std::unique_lock lock{ m_mutex };
    if (m_needs_recovery) recover_locked();
    while (!m_stopped && !m_error && !m_control->paused && (m_clearing || m_id.size() < count))
    {
        if (cancelled && cancelled->load())
            throw std::runtime_error{"Task cancelled."};
        m_condition_variable.wait_for(lock, std::chrono::milliseconds{50});
    }
    if (cancelled && cancelled->load())
        throw std::runtime_error{"Task cancelled."};
    if (m_stopped) throw std::runtime_error{ "Blob queue stopped." };
    if (m_error) std::rethrow_exception(m_error);
    if (m_control->paused)
        throw std::runtime_error{m_control->reason()};
    std::vector<std::string> out{ m_id.end()-count, m_id.end() };
    trim_locked(count);
    return out;
}

auto blob_queue_t::check_error() const -> void
{
    std::unique_lock lock{ m_mutex };
    if (m_error) std::rethrow_exception(m_error);
    if (m_control->paused)
        throw std::runtime_error{m_control->reason()};
}

auto blob_queue_t::count() -> std::uint64_t
{
    std::unique_lock lock{ m_mutex };
    return m_id.size();
}

auto blob_queue_t::cycle_rate() const -> std::uint64_t
{
    std::unique_lock lock{ m_mutex };
    return m_cycle_time.size();
}

auto blob_queue_t::clear_begin() -> void
{
    std::unique_lock lock{ m_mutex };
    m_clearing = true;
    return;
}

auto blob_queue_t::clear_end() -> void
{
    std::unique_lock lock{ m_mutex };
    if (m_needs_recovery) recover_locked();
    trim_locked(m_id.size());
    m_cleared = true;
    m_clearing = false;
    lock.unlock();
    m_condition_variable.notify_all();
    return;
}

auto blob_queue_t::recover_locked() -> void
{
    auto journal=m_path; journal+=".tail";
    if (std::filesystem::exists(journal))
    {
        const auto edit=nlohmann::json::parse(storage_read(journal));
        storage_rewrite_tail(m_path,edit.at("offset"),edit.at("tail").get<std::string>());
    }
    if (!std::filesystem::exists(m_path)) storage_atomic_write(m_path,"[]");
    const auto content=storage_read(m_path);
    const auto ids=nlohmann::json::parse(content).get<std::deque<std::string>>();
    m_file_size=content.size();
    // Existing queues can contain whitespace. Canonicalize once at load;
    // subsequent edits serialize only the IDs being appended or removed.
    const auto canonical=nlohmann::json(ids).dump();
    if (content!=canonical) write_tail_locked(0,canonical);
    m_id=ids;
    m_needs_recovery=false;
}
auto blob_queue_t::write_tail_locked(std::uint64_t offset, const std::string& tail) -> void
{
    auto journal=m_path; journal+=".tail";
    m_needs_recovery=true;
    // Keep the latest edit even after success: replay is idempotent. It also
    // repairs a truncated/partially overwritten JSON suffix after termination.
    // Replacing this small journal durably always precedes editing live bytes.
    storage_atomic_write(journal,nlohmann::json{{"offset",offset},{"tail",tail}}.dump());
    storage_rewrite_tail(m_path,offset,tail);
    m_file_size=offset+tail.size();
    m_needs_recovery=false;
}
auto blob_queue_t::append_locked(const std::vector<std::string>& ids) -> void
{
    if (m_needs_recovery) recover_locked();
    if (ids.empty()) return;
    auto tail=nlohmann::json(ids).dump().substr(1);
    if (!m_id.empty()) tail.insert(tail.begin(),',');
    write_tail_locked(m_file_size-1,tail);
    m_id.insert(m_id.end(),ids.begin(),ids.end());
}
auto blob_queue_t::trim_locked(std::size_t count) -> void
{
    if (!count) return;
    auto size=m_file_size;
    for (auto entry=m_id.end()-count;entry!=m_id.end();++entry)
        size-=nlohmann::json(*entry).dump().size()+1;
    if (count==m_id.size()) size=2;
    write_tail_locked(size-1,"]");
    m_id.erase(m_id.end()-count,m_id.end());
}

auto blob_queue_t::retry() -> void
{
    m_account->retry();
    m_control->retry();
}
