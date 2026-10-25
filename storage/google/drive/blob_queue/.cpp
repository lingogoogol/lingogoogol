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

    curl_multi_t& multi{ curl_multi_t::get() };
	//A array of easy handle that is currently not used.
	std::vector<google_batch_t> batch_idle_arr( 64 );
    for (auto& batch : batch_idle_arr)
    {
        batch = google_batch_t{ curl_easy_t{} };
    }

    std::chrono::steady_clock::time_point last_cycle_time{};
    while (!*m_close && !m_stopped)
    {
        std::this_thread::sleep_until(last_cycle_time + 0.1s);
        std::chrono::high_resolution_clock::time_point now{ std::chrono::high_resolution_clock::now() };
        last_cycle_time = now;
        m_cycle_time.push_back(now);
        while (m_cycle_time.front() <= now - 1s)
        {
            m_cycle_time.pop_front();
        }

        multi.run();
        while (true)
        {
            if (!multi.ready_nonempty())
            {
                break;
            }
            google_batch_t batch{ multi.ready_get<google_batch_t>() };
            std::unique_lock lock{ m_mutex };
            if (!m_cleared)
            {
                std::uint64_t size_orig{ m_id.size() };
                m_id.resize(size_orig + 100);
                for (std::uint64_t i{ 0 }; i < 100; ++i)
                {
                    m_id[size_orig + i] = nlohmann::json::parse(batch.response_get(i).body_get())["id"];
                }
                lock.unlock();
                m_condition_variable.notify_all();
            }
            batch.clear();
            m_cleared = false;
            batch_idle_arr.push_back(std::move(batch));
        }

        std::unique_lock lock{ m_mutex };
        if (m_id.size() >= 20000)
        {
            continue;
        }
        lock.unlock();
        while (batch_idle_arr.size())
        {
            google_batch_t batch{ std::move(batch_idle_arr.back()) };
            batch_idle_arr.pop_back();

            batch.path_set("/drive/v3");
            for (std::uint64_t i{ 0 }; i < 100; ++i)
            {
                blob_upload(batch, "", "");
            }
            batch.method_post();
            multi.add(std::move(batch));
        }
    }
    return;
}

blob_queue_t::blob_queue_t(std::filesystem::path path, google_account_t* account, const std::atomic_bool* close)
: m_path{ path }, m_account{ account }, m_close{ close }
{
    if (std::filesystem::is_regular_file(path))
    {
        std::uint64_t size{ std::filesystem::file_size(path) };
        std::string content{};
        content.resize(size);
        lgo::create_ifstream(path.string()).read(content.data(), size);
        std::filesystem::remove(path);
        //Use `=` instead of `{}`: `nlohmann::json{ expr }` invokes the `std::initializer_list` constructor
        //and wraps `expr` into a 1-element JSON array instead of copy/move-constructing the JSON value.
        nlohmann::json arr = nlohmann::json::parse(content);
        m_id.resize(arr.size());
        for (std::uint64_t i{ 0 }; i < arr.size(); ++i)
        {
            m_id[i] = arr[i];
        }
    }
    m_thread = std::thread{ std::bind(&blob_queue_t::process, this) };
    return;
}

auto blob_queue_t::stop() -> void
{
    // Publish cancellation under the same mutex as get()'s predicate to avoid lost wakeups.
    {
        std::unique_lock lock{ m_mutex };
        m_stopped = true;
    }
    m_condition_variable.notify_all();
}

blob_queue_t::~blob_queue_t()
{
    stop();
    m_thread.join();
    //Use `=` instead of `{}` to avoid wrapping the empty array into `[[]]`.
    nlohmann::json arr = nlohmann::json::array();
    for (std::uint64_t i{ 0 }; i < m_id.size(); ++i)
    {
        arr.push_back(m_id[i]);
    }
    std::string content{ arr.dump() };
    lgo::create_ofstream_destroy(m_path.string()).write(content.data(), content.size());
    return;
}

auto blob_queue_t::get() -> std::string
{
    std::unique_lock lock{ m_mutex };
    m_condition_variable.wait(lock, [this] () -> bool { return m_stopped || (!m_clearing && m_id.size() > 1); });
    if (m_stopped) throw std::runtime_error{ "Blob queue stopped." };
    std::string out{ m_id.front() };
    m_id.pop_front();
    return out;
}

auto blob_queue_t::get(std::uint64_t count) -> std::vector<std::string>
{
    std::unique_lock lock{ m_mutex };
    m_condition_variable.wait(lock, [this, count] () -> bool { return m_stopped || (!m_clearing && m_id.size() >= count); });
    if (m_stopped) throw std::runtime_error{ "Blob queue stopped." };
    std::vector<std::string> out{ m_id.begin(), m_id.begin() + count };
    m_id.erase(m_id.begin(), m_id.begin() + count);
    return out;
}

auto blob_queue_t::count() -> std::uint64_t
{
    std::unique_lock lock{ m_mutex };
    return m_id.size();
}

auto blob_queue_t::cycle_rate() const -> std::uint64_t
{
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
    m_id.clear();
    m_cleared = true;
    m_clearing = false;
    lock.unlock();
    m_condition_variable.notify_all();
    return;
}
