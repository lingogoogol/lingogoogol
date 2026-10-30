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
    if (!ok ||
        !MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error{"Cannot save checkpoint: " + path.string()};
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
    result.signed_out_service_accounts = data.value("signed_out_service_accounts", result.signed_out_service_accounts);
    if (data.contains("signed_in_accounts"))
    {
        result.signed_in_accounts.emplace();
        const auto& accounts = data.at("signed_in_accounts");
        if (!accounts.is_object()) throw std::runtime_error{"Invalid saved account login list."};
        for (const auto& [id,account] : accounts.items())
            result.signed_in_accounts->emplace(id,storage_account_login_t{
                account.at("address").get<std::string>(),account.at("account_path").get<std::string>(),
                account.at("root_directory").get<std::string>(),account.at("oauth").get<bool>()});
        result.selected_account = data.value("selected_account",std::string{});
    }
    // An intermediate layout saved the old default filename as root/root.
    // root_path now names a directory; keep that default from nesting again.
    if (std::filesystem::path{result.root_path}.lexically_normal()==std::filesystem::path{"root/root"})
        result.root_path = "root";
    result.network_retries = data.value("network_retries", -1);
    if (result.network_retries < -1)
        throw std::runtime_error{"Network retries must be nonnegative or -1 (unlimited)."};
    return result;
}
auto storage_settings_t::save() const -> void
{
    auto data = nlohmann::json{{"logfile_path", logfile_path},
                                                         {"font_path", font_path},
                                                         {"account_path", account_path},
                                                         {"root_path", root_path},
                                                         {"signed_out_service_accounts", signed_out_service_accounts},
                                                         {"network_retries", network_retries}};
    if (signed_in_accounts)
    {
        auto accounts = nlohmann::json::object();
        for (const auto& [id,account] : *signed_in_accounts)
            accounts[id] = {{"address",account.address},{"account_path",account.account_path},
                            {"root_directory",account.root_directory},{"oauth",account.oauth}};
        data["signed_in_accounts"] = std::move(accounts);
        data["selected_account"] = selected_account;
    }
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
}
auto task_journal_t::save_locked() -> void
{
    storage_atomic_write(m_path / "state", m_data.dump());
}
auto task_journal_t::data() const -> nlohmann::json
{
    std::lock_guard lock{m_mutex};
    return m_data;
}
auto task_journal_t::set(std::string key, nlohmann::json value) -> void
{
    std::lock_guard lock{m_mutex};
    if (m_finished)
        return;
    m_data[std::move(key)] = std::move(value);
    save_locked();
}
auto task_journal_t::allocate(std::string key, std::size_t count,
                              const std::function<std::vector<std::string>(std::size_t)>& reserve)
    -> std::vector<std::string>
{
    std::lock_guard lock{m_mutex};
    if (m_finished)
        throw std::runtime_error{"Task cancelled."};
    auto& entries{m_data["allocations"]};
    if (entries.contains(key))
    {
        auto result{entries.at(key).get<std::vector<std::string>>()};
        if (result.size() != count)
            throw std::runtime_error{"Upload source changed since the saved task."};
        return result;
    }
    auto result{reserve(count)};
    entries[std::move(key)] = result;
    save_locked();
    return result;
}
auto task_journal_t::response(std::string_view key) const -> std::optional<std::string>
{
    std::lock_guard lock{m_mutex};
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
    // Keep orphan data until the index is durable; interrupted cleanup is harmless.
    std::filesystem::remove_all(m_path);
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
