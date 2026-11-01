module;
#include <nlohmann/json.hpp>
module storage.kernel.load;
import std;

auto storage_session_t::remember(const std::string& id, const std::string& address, std::string type) -> void
{
    if (id == "null") return;
    std::lock_guard lock{routing_mutex};
    owners[id] = address;
    if (!type.empty()) kinds[id] = std::move(type);
}
auto storage_session_t::owner(const std::string& id) const -> std::string
{
    if (id == "null") return "null";
    std::lock_guard lock{routing_mutex};
    return owners.at(id);
}
auto storage_session_t::kind(const std::string& id) const -> std::string
{
    std::lock_guard lock{routing_mutex};
    return kinds.at(id);
}
auto storage_session_t::bind(curl_easy_t& request, const std::string& id) const -> void
{
    const auto address=owner(id);
    const auto account=accounts.find(address);
    if (account==accounts.end()) throw std::runtime_error{"此 blob 的服務帳戶未關聯到空間："+address};
    request.m_account_context=account->second;
}
auto storage_session_t::observe(const std::string& id, const nlohmann::json& name,
                                const nlohmann::json& content) -> void
{
    const auto type = kind(id);
    remember(name.at("parent"),name.at("parent_account"),type == "data" ? "chunk" : type == "chunk" ? "file" : "folder");
    if (name.contains("next")) remember(name.at("next"),name.at("next_account"),type);
    if (!content.is_array()) return;
    for (const auto& child : content)
        if (type == "folder") remember(child.at("id"),child.at("account"),child.at("type"));
        else remember(child.get<std::string>(),name.at("account"),type == "file" ? "chunk" : "data");
}
auto storage_session_t::choose(std::uint64_t units) -> std::string
{
    std::lock_guard lock{routing_mutex};
    if (accounts.empty()) throw std::runtime_error{"請先關聯服務帳戶"};
    // Compare project load before per-user load. A flat round-robin over sorted
    // addresses would exhaust one project's quota before visiting the next.
    std::map<std::string,std::uint64_t> background_accounts, background_projects;
    for (const auto& [address, unused] : accounts)
    {
        const auto queue=queues.find(address);
        const auto created=queue==queues.end() ? 0 : queue->second->planned_creations();
        background_accounts[address]=created;
        background_projects[address.substr(address.find('@')+1)]+=created;
    }
    std::optional<std::tuple<std::uint64_t,std::uint64_t,std::string>> best;
    for (const auto& [address, unused] : accounts)
    {
        const auto project = address.substr(address.find('@')+1);
        // Allocation and reserve creation are both logical writes (currently
        // 50 Drive quota units each). Read one snapshot so a concurrent refill
        // cannot change a project's total halfway through candidate comparison.
        auto rank = std::tuple{project_units[project]+background_projects[project],
            account_units[address]+background_accounts[address],address};
        if (!best || rank < *best) best = std::move(rank);
    }
    const auto address = std::get<2>(*best);
    project_units[address.substr(address.find('@')+1)] += units;
    account_units[address] += units;
    return address;
}
auto storage_session_t::routing() const -> nlohmann::json
{
    std::lock_guard lock{routing_mutex};
    return {{"owners",owners},{"kinds",kinds}};
}
auto storage_session_t::restore_routing(const nlohmann::json& saved) -> void
{
    std::lock_guard lock{routing_mutex};
    for (const auto& [id,address] : saved.at("owners").items()) owners[id] = address;
    for (const auto& [id,type] : saved.at("kinds").items()) kinds[id] = type;
}
auto task_owner(load_task_t& task, const std::string& key, std::uint64_t units) -> std::string
{
    const auto field = "owner:" + key;
    if (task.m_journal)
    {
        if (const auto saved=task.m_journal->field(field)) return saved->get<std::string>();
    }
    auto address = task.m_session->choose(units);
    // Persist the identity before reserving IDs. A crash must never replay a
    // reserved ID against another account, even after project load has changed.
    if (task.m_journal) task.m_journal->set(field,address);
    return address;
}
auto task_reserve(load_task_t& task, const std::string& key, std::size_t count,
                   std::string address, std::string type) -> std::vector<std::string>
{
    if (address.empty()) address = task_owner(task,key,count);
    auto reserve = [&](std::size_t size)
        { return task.m_session->queues.at(address)->get(size,&task.m_control->cancelled); };
    auto ids = task.m_journal ? task.m_journal->allocate(key,count,reserve) : reserve(count);
    for (const auto& id : ids) task.m_session->remember(id,address,type);
    if (task.m_journal) task.m_journal->record_routing(ids,address,type);
    return ids;
}
