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
    google_batch_t batch{ curl_easy_t{} };
    while (!*m_close)
    {
        batch.path_set("/drive/v3");
        std::unique_lock lock{ m_mutex };
        if (m_id.size() >= 200)
        {
            continue;
        }
        bool clear_copy{ m_cleared };
        lock.unlock();
        for (std::uint64_t i{ 0 }; i < 100; ++i)
        {
            blob_upload(batch, "", "");
        }
        batch.method_post();
        batch.run();
        lock.lock();
        if (!m_cleared || clear_copy)
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
        nlohmann::json arr{ nlohmann::json::parse(content) };
        m_id.resize(arr.size());
        for (std::uint64_t i{ 0 }; i < arr.size(); ++i)
        {
            m_id[i] = arr[i];
        }
    }
    m_thread = std::thread{ std::bind(&blob_queue_t::process, this) };
    return;
}

blob_queue_t::~blob_queue_t()
{
    m_thread.join();
    nlohmann::json arr{ nlohmann::json::array() };
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
    m_condition_variable.wait(lock, [this] () -> bool { return !m_clearing && m_id.size() > 1; });
    std::string out{ m_id.front() };
    m_id.pop_front();
    return out;
}

auto blob_queue_t::get(std::uint64_t count) -> std::vector<std::string>
{
    std::unique_lock lock{ m_mutex };
    m_condition_variable.wait(lock, [this, count] () -> bool { return !m_clearing && m_id.size() >= count; });
    std::vector<std::string> out{ m_id.begin(), m_id.begin() + count };
    m_id.erase(m_id.begin(), m_id.begin() + count);
    return out;
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
