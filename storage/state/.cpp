module;
#include <nlohmann/json.hpp>
#include <winsock2.h>
#include <windows.h>
#include <openssl/sha.h>
module storage.state;
import std;

namespace
{
    std::mutex index_mutex;

    // A single worker owns only paths, never task objects or GUI state. In
    // particular, destroying a completed row must not join a per-task future
    // and move a long directory deletion back onto the GUI thread.
    class journal_cleanup_t
    {
        std::mutex m_mutex;
        std::condition_variable_any m_ready;
        std::deque<std::filesystem::path> m_paths;
        // Declared last so jthread stops/joins before its queue and locks die.
        // Drain queued paths on normal exit; an interrupted deletion only
        // leaves unindexed cache files, never a resumable completed task.
        std::jthread m_worker{[this](std::stop_token stop)
        {
            for (;;)
            {
                std::unique_lock lock{m_mutex};
                m_ready.wait(lock,stop,[this] { return !m_paths.empty(); });
                if (m_paths.empty()) return;
                auto path=std::move(m_paths.front());
                m_paths.pop_front();
                lock.unlock();
                std::error_code error;
                std::filesystem::remove_all(path,error);
                // A locked cache can remain orphaned. It must not turn an
                // already durable successful transfer into a failed task.
                if (error)
                    OutputDebugStringW((L"Cannot remove completed task cache: "+path.wstring()+
                                        L" (Windows error "+std::to_wstring(error.value())+L")\n").c_str());
            }
        }};

      public:
        auto add(std::filesystem::path path) -> void
        {
            {
                std::lock_guard lock{m_mutex};
                m_paths.push_back(std::move(path));
            }
            m_ready.notify_one();
        }
    };

    auto journal_cleanup() -> journal_cleanup_t&
    {
        static journal_cleanup_t cleanup;
        return cleanup;
    }

    auto update_index(const std::string& id, bool present) -> void
    {
        std::lock_guard lock{index_mutex};
        auto index = nlohmann::json::array();
        if (std::filesystem::exists("task"))
            index = nlohmann::json::parse(storage_read("task"));
        if (!index.is_array())
            throw std::runtime_error{"Invalid task index; original file was preserved."};
        std::erase_if(index.get_ref<nlohmann::json::array_t&>(),
                      [&](const auto& entry) { return entry == id; });
        if (present)
            index.push_back(id);
        storage_atomic_write("task", index.dump());
    }
} // namespace
auto storage_flush_file(const std::filesystem::path& path) -> void
{
    const auto handle{CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error{"Cannot flush download: " + path.string()};
    const auto ok{FlushFileBuffers(handle)};
    CloseHandle(handle);
    if (!ok)
        throw std::runtime_error{"Cannot flush download: " + path.string()};
}
auto storage_read(const std::filesystem::path& path) -> std::string
{
    std::ifstream stream{path, std::ios::binary};
    if (!stream)
        throw std::runtime_error{"Cannot read: " + path.string()};
    return {std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
}
auto storage_rewrite_tail(const std::filesystem::path& path, std::uint64_t offset,
                          std::string_view tail) -> void
{
    const auto handle=CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,
                                 OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (handle==INVALID_HANDLE_VALUE) throw std::runtime_error{"Cannot edit queue: "+path.string()};
    struct close_t { HANDLE handle; ~close_t() { CloseHandle(handle); } } close{handle};
    LARGE_INTEGER position;
    position.QuadPart=static_cast<LONGLONG>(offset);
    if (!SetFilePointerEx(handle,position,nullptr,FILE_BEGIN))
        throw std::runtime_error{"Cannot seek queue: "+path.string()};
    while (!tail.empty())
    {
        DWORD written{};
        const auto count=static_cast<DWORD>(std::min<std::size_t>(tail.size(),0x40000000));
        if (!WriteFile(handle,tail.data(),count,&written,nullptr) || !written)
            throw std::runtime_error{"Cannot write queue tail: "+path.string()};
        tail.remove_prefix(written);
    }
    if (!SetEndOfFile(handle) || !FlushFileBuffers(handle))
        throw std::runtime_error{"Cannot flush queue tail: "+path.string()};
}
auto storage_atomic_write(const std::filesystem::path& path, std::string_view content) -> void
{
    if (path.has_parent_path())
        std::filesystem::create_directories(path.parent_path());
    auto temporary{path};
    temporary += ".tmp";
    const auto handle{CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error{"Cannot create checkpoint: " + path.string()};
    bool ok{true};
    while (!content.empty())
    {
        DWORD written{};
        const auto count{static_cast<DWORD>(std::min<std::size_t>(content.size(), 0x40000000))};
        if (!WriteFile(handle, content.data(), count, &written, nullptr) || written == 0)
        {
            ok = false;
            break;
        }
        content.remove_prefix(written);
    }
    if (ok)
        ok = FlushFileBuffers(handle);
    CloseHandle(handle);
    if (!ok) throw std::runtime_error{"Cannot flush checkpoint: " + path.string()};
    // External readers may omit FILE_SHARE_DELETE. Keep the old checkpoint
    // intact and tolerate short sharing/antivirus locks during atomic replace.
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds{2};
    while (!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        const auto error=GetLastError();
        if ((error!=ERROR_SHARING_VIOLATION && error!=ERROR_LOCK_VIOLATION && error!=ERROR_ACCESS_DENIED) ||
            std::chrono::steady_clock::now()>=deadline)
            throw std::runtime_error{"Cannot save checkpoint: "+path.string()+" (Windows error "+std::to_string(error)+")"};
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
}
auto storage_digest(std::string_view content) -> std::string
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(content.data()), content.size(), digest);
    std::string result;
    for (auto byte : digest)
        result += std::format("{:02x}", byte);
    return result;
}
auto storage_settings_t::parse_http_log_codes(std::string_view text) -> std::set<long>
{
    std::set<long> codes;
    while (!text.empty())
    {
        const auto end = text.find_first_of(", \t\r\n");
        const auto token = text.substr(0, end);
        if (!token.empty())
        {
            long code{};
            const auto [last, error] = std::from_chars(token.data(), token.data() + token.size(), code);
            if (error != std::errc{} || last != token.data() + token.size())
                throw std::runtime_error{"HTTP 狀態碼須為可表示的整數，以逗號或空白分隔；留空停用"};
            codes.insert(code);
        }
        if (end == std::string_view::npos) break;
        text.remove_prefix(end + 1);
    }
    return codes;
}
auto storage_settings_t::parse_worker_count(std::string_view text) -> std::size_t
{
    std::size_t count{};
    const auto [end,error] = std::from_chars(text.data(),text.data()+text.size(),count);
    if (error!=std::errc{} || end!=text.data()+text.size() || !count)
        throw std::runtime_error{"worker 數量必須是有效的正整數"};
    return count;
}

auto storage_settings_t::parse_batch_size(std::string_view text) -> std::size_t
{
    std::size_t count{};
    const auto [end,error] = std::from_chars(text.data(),text.data()+text.size(),count);
    if (error!=std::errc{} || end!=text.data()+text.size() || count<1 || count>100)
        throw std::runtime_error{"batch_size 必須是 1 到 100 的整數"};
    return count;
}

auto storage_settings_t::load() -> storage_settings_t
{
    storage_settings_t result;
    if (!std::filesystem::exists("settings.json"))
        return result;
    const auto data = nlohmann::json::parse(storage_read("settings.json"));
    result.logfile_path = data.value("logfile_path", result.logfile_path);
    result.font_path = data.value("font_path", result.font_path);
    result.account_path = data.value("account_path", result.account_path);
    result.root_path = data.value("root_path", result.root_path);
    for (const auto& space : data.value("spaces", nlohmann::json::array()))
        result.spaces.push_back({space.at("id"),space.at("name"),
            space.at("accounts").get<std::vector<std::string>>(),space.at("signed_in")});
    result.selected_space = data.value("selected_space",std::string{});
    result.http_log_codes = data.value("http_log_codes", std::string{});
    parse_http_log_codes(result.http_log_codes);
    result.batch_request = data.value("batch_request", true);
    result.batch_gzip = data.value("batch_gzip", false);
    result.content_zstd = data.value("content_zstd", false);
    // Parse the JSON representation as an integer too: typed JSON conversion
    // to size_t would silently wrap negative values and truncate decimals.
    result.worker_count = parse_worker_count(data.value("worker_count",nlohmann::json(64)).dump());
    result.batch_size = parse_batch_size(data.value("batch_size",nlohmann::json(100)).dump());
    result.network_retries = data.value("network_retries", -1);
    if (result.network_retries < -1)
        throw std::runtime_error{"Network retries must be nonnegative or -1 (unlimited)."};
    return result;
}
auto storage_settings_t::save() const -> void
{
    parse_http_log_codes(http_log_codes);
    if (!worker_count) throw std::runtime_error{"worker 數量必須大於零"};
    if (batch_size<1 || batch_size>100) throw std::runtime_error{"batch_size 必須是 1 到 100 的整數"};
    auto data = nlohmann::json{{"logfile_path", logfile_path},
                                                         {"http_log_codes", http_log_codes},
                                                         {"font_path", font_path},
                                                         {"account_path", account_path},
                                                         {"root_path", root_path},
                                                         {"network_retries", network_retries},
                                                         {"batch_request", batch_request},
                                                         {"batch_gzip", batch_gzip},
                                                         {"content_zstd", content_zstd},
                                                         {"batch_size", batch_size},
                                                         {"worker_count", worker_count}};
    data["spaces"] = nlohmann::json::array();
    for (const auto& space : spaces)
        data["spaces"].push_back({{"id",space.id},{"name",space.name},
            {"accounts",space.accounts},{"signed_in",space.signed_in}});
    data["selected_space"] = selected_space;
    storage_atomic_write("settings.json",data.dump(2));
}
auto request_control_t::fail(std::string reason) -> void
{
    std::lock_guard lock{mutex};
    error = std::move(reason);
    paused = true;
}
auto request_control_t::retry() -> void
{
    std::lock_guard lock{mutex};
    error.clear();
    ++epoch;
    paused = false;
}
auto request_control_t::reason() const -> std::string
{
    std::lock_guard lock{mutex};
    return error;
}

task_journal_t::task_journal_t(std::filesystem::path path, nlohmann::json intent)
    : m_path{std::move(path)}, m_data(std::move(intent))
{
    m_initial_saved=std::filesystem::exists(m_path/"state");
    if (!m_initial_saved) return;
    std::vector<std::filesystem::path> records;
    for (const auto& entry : std::filesystem::directory_iterator{m_path})
        if (entry.path().extension()==".record") records.push_back(entry.path());
    std::ranges::sort(records);
    for (const auto& record : records)
    {
        const auto stem=record.stem().string();
        std::uint64_t sequence{};
        const auto [end,error]=std::from_chars(stem.data(),stem.data()+stem.size(),sequence);
        if (error!=std::errc{} || end!=stem.data()+stem.size() || sequence!=m_next_record)
            throw std::runtime_error{"Invalid task record sequence: "+record.string()};
        const auto changes=nlohmann::json::parse(storage_read(record));
        if (changes.contains("fields"))
            for (const auto& [key,value] : changes.at("fields").items()) m_data[key]=value;
        if (changes.contains("allocations"))
            for (const auto& [key,value] : changes.at("allocations").items()) m_data["allocations"][key]=value;
        if (changes.contains("routing"))
            for (const auto& [kind,entries] : changes.at("routing").items())
                for (const auto& [id,value] : entries.items()) m_data["routing"][kind][id]=value;
        ++m_next_record;
    }
}
auto task_journal_t::save_locked() -> void
{
    if (!m_initial_saved)
    {
        storage_atomic_write(m_path/"state",m_data.dump());
        m_initial_saved=true;
    }
    else
    {
        // Persist only this change, never the accumulated entity/routing table.
        // Atomic files make interrupted writes ignorable (.tmp); the initial
        // intent plus the ordered records reconstruct the in-memory snapshot.
        storage_atomic_write(m_path/std::format("{:020}.record",m_next_record),m_pending.dump());
        ++m_next_record;
    }
    m_pending=nlohmann::json::object();
    m_dirty=false;
}
auto task_journal_t::data() const -> nlohmann::json
{
    std::lock_guard lock{m_mutex};
    return m_data;
}
auto task_journal_t::field(std::string_view key) const -> std::optional<nlohmann::json>
{
    std::lock_guard lock{m_mutex};
    const auto found=m_data.find(std::string{key});
    return found==m_data.end() ? std::nullopt : std::optional<nlohmann::json>{*found};
}
auto task_journal_t::progress(std::string_view key) const -> std::optional<nlohmann::json>
{
    std::lock_guard lock{m_mutex};
    if (m_finished) return std::nullopt;
    const auto path=m_path/(storage_digest(key)+".progress");
    if (!std::filesystem::exists(path)) return std::nullopt;
    return nlohmann::json::parse(storage_read(path));
}
auto task_journal_t::progress(std::string_view key, const nlohmann::json& value) -> void
{
    std::lock_guard lock{m_mutex};
    if (!m_finished) storage_atomic_write(m_path/(storage_digest(key)+".progress"),value.dump());
}
auto task_journal_t::record_routing(const std::vector<std::string>& ids, const std::string& address,
                                   const std::string& type) -> void
{
    std::lock_guard lock{m_mutex};
    if (m_finished) return;
    for (const auto& id : ids)
    {
        auto& owner=m_data["routing"]["owners"][id];
        if (owner!=address)
        { owner=address; m_pending["routing"]["owners"][id]=address; m_dirty=true; }
        if (!type.empty())
        {
            auto& kind=m_data["routing"]["kinds"][id];
            if (kind!=type)
            { kind=type; m_pending["routing"]["kinds"][id]=type; m_dirty=true; }
        }
    }
    if (m_dirty) save_locked();
}
auto task_journal_t::set(std::string key, nlohmann::json value) -> void
{
    std::lock_guard lock{m_mutex};
    if (m_finished)
        return;
    const auto found=m_data.find(key);
    if (found==m_data.end() || *found!=value)
    { m_pending["fields"][key]=value; m_data[std::move(key)] = std::move(value); m_dirty=true; }
    // A failed flush must remain dirty even if a retry supplies the same value.
    if (m_dirty) save_locked();
}
auto task_journal_t::allocate(std::string key, std::size_t count,
                              const std::function<std::vector<std::string>(std::size_t)>& reserve,
                              const nlohmann::json& fields, const std::string& address, const std::string& type)
    -> std::vector<std::string>
{
    std::lock_guard lock{m_mutex};
    if (m_finished)
        throw std::runtime_error{"Task cancelled."};
    auto& entries{m_data["allocations"]};
    std::vector<std::string> result;
    if (entries.contains(key))
    {
        result=entries.at(key).get<std::vector<std::string>>();
        if (result.size() != count)
            throw std::runtime_error{"Upload source changed since the saved task."};
    }
    else
    {
        result=reserve(count);
        if (result.size()!=count) throw std::runtime_error{"Incomplete ID reservation."};
        m_pending["allocations"][key]=result;
        entries[std::move(key)]=result;
        m_dirty=true;
    }
    for (const auto& [field,value] : fields.items())
        if (!m_data.contains(field) || m_data.at(field)!=value)
        { m_data[field]=value; m_pending["fields"][field]=value; m_dirty=true; }
    if (!address.empty())
        for (const auto& id : result)
        {
            for (const auto& [kind,value] : std::array<std::pair<std::string,std::string>,2>{{{"owners",address},{"kinds",type}}})
            {
                if (value.empty()) continue;
                auto& stored=m_data["routing"][kind][id];
                if (stored!=value)
                { stored=value; m_pending["routing"][kind][id]=value; m_dirty=true; }
            }
        }
    if (m_dirty) save_locked();
    return result;
}
auto task_journal_t::response(std::string_view key) const -> std::optional<std::string>
{
    std::lock_guard lock{m_mutex};
    if (m_finished) return std::nullopt;
    const auto path{m_path / (std::string{key} + ".response")};
    if (!std::filesystem::exists(path))
        return std::nullopt;
    return storage_read(path);
}
auto task_journal_t::response(std::string_view key, std::string_view value) -> void
{
    std::lock_guard lock{m_mutex};
    if (m_finished)
        return;
    storage_atomic_write(m_path / (std::string{key} + ".response"), value);
}
auto task_journal_t::finish() -> void
{
    std::lock_guard lock{m_mutex};
    if (m_finished)
        return;
    update_index(m_path.filename().string(), false);
    m_finished = true;
    // The small index must be durable before publishing completion. Removing
    // tens of thousands of response files must not block input/rendering.
    // m_finished and the journal mutex prevent late writers from recreating
    // files while the independent worker deletes them.
    journal_cleanup().add(std::filesystem::absolute(m_path));
}
auto task_journal_t::create(nlohmann::json intent) -> std::shared_ptr<task_journal_t>
{
    static std::atomic_uint64_t serial{};
    const auto id{std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "-" +
                  std::to_string(++serial)};
    auto result{std::make_shared<task_journal_t>(std::filesystem::path{"task.data"} / id, std::move(intent))};
    {
        std::lock_guard lock{result->m_mutex};
        result->save_locked();
    }
    update_index(id, true);
    return result;
}
auto task_journal_t::restore() -> std::vector<std::shared_ptr<task_journal_t>>
{
    std::vector<std::shared_ptr<task_journal_t>> result;
    if (!std::filesystem::exists("task"))
        return result;
    const auto index = nlohmann::json::parse(storage_read("task"));
    if (!index.is_array())
        throw std::runtime_error{"Invalid task index; original file was preserved."};
    for (const auto& entry : index)
    {
        const auto id{entry.get<std::string>()};
        if (id.empty() || id.find_first_not_of("0123456789-") != std::string::npos)
            throw std::runtime_error{"Invalid saved task identifier."};
        const auto path{std::filesystem::path{"task.data"} / id};
        result.push_back(
            std::make_shared<task_journal_t>(path, nlohmann::json::parse(storage_read(path / "state"))));
    }
    return result;
}

auto request_control_t::cancel() -> void
{
    std::lock_guard lock{mutex};
    cancelled = true;
    for (const auto socket : sockets)
        ::shutdown(static_cast<SOCKET>(socket), SD_BOTH);
}
auto request_control_t::pause(bool value) -> void
{
    std::lock_guard lock{mutex};
    if (user_paused.exchange(value) == value) return;
    if (value)
    {
        // Generation survives a rapid Pause/Continue before the curl owner wakes.
        ++pause_epoch;
        for (const auto socket : sockets)
            ::shutdown(static_cast<SOCKET>(socket), SD_BOTH);
    }
}
auto request_control_t::socket_open(int family, int type, int protocol) -> std::uintptr_t
{
    std::lock_guard lock{mutex};
    if (cancelled || user_paused)
        return INVALID_SOCKET;
    const auto socket{::socket(family, type, protocol)};
    if (socket != INVALID_SOCKET)
        sockets.insert(static_cast<std::uintptr_t>(socket));
    return static_cast<std::uintptr_t>(socket);
}
auto request_control_t::socket_close(std::uintptr_t socket) -> int
{
    std::lock_guard lock{mutex};
    sockets.erase(socket);
    return ::closesocket(static_cast<SOCKET>(socket));
}
