module;
#include <nlohmann/json.hpp>
module storage.kernel.load;
import std;
import storage.http;
import storage.google;

namespace
{
    struct request_group_t
    {
        std::unique_ptr<google_batch_t> request;
        std::function<void(std::unique_ptr<curl_easy_t>&)> complete;
        std::size_t remaining{};
    };

    auto submit_part(load_state_t& state, curl_multi_worker_t& multi, curl_easy_t&& transport,
                     const std::shared_ptr<request_group_t>& group, std::size_t index) -> void
    {
        google_batch_t request{std::move(transport)};
        request.clear();
        request.m_account_context = group->request->m_account_context;
        request.control_set(group->request->m_control);
        request.m_journal = group->request->m_journal;
        request.host_set(group->request->host_get());
        request.request_add();
        request.request_get(0) = group->request->request_get(index);
        request.method_single();
        const auto key = request.interface_get();
        state.m_end.emplace(key,[group,index](auto& result)
        {
            auto& part = dynamic_cast<google_batch_t&>(*result);
            group->request->response_get(index) = part.response_get(0);
            if (--group->remaining) return;
            // Keep the same grouped checkpoint and completion contract as batch
            // mode. Each successful child also has its own durable response, so
            // a crash inside a group only resends its unfinished requests.
            group->request->restore_response(group->request->saved_response());
            group->request->checkpoint();
            // A successful child parked during Pause already returned its
            // handle to the pool. Its deferred callback must stay handle-free.
            if (auto handle=result->take_transport()) group->request->bind_transport(handle);
            std::unique_ptr<curl_easy_t> completed{std::move(group->request)};
            group->complete(completed);
            result = std::move(completed);
        });
        multi.add(std::move(request));
    }
}

auto task_batch_submit(load_state_t& state, curl_multi_worker_t& multi, google_batch_t&& request) -> void
{
    if (state.m_settings.batch_request || request.replay())
    {
        multi.add(std::move(request));
        return;
    }
    auto callback = state.m_end.extract(request.interface_get());
    if (callback.empty()) throw std::logic_error{"Missing task request callback."};
    auto group = std::make_shared<request_group_t>();
    group->remaining = request.part_count();
    group->complete = std::move(callback.mapped());
    group->request = std::make_unique<google_batch_t>(std::move(request));
    // Pending descriptions own no handles. The same GUI pool limits actual
    // HTTP concurrency, even when many content groups are split at once.
    for (std::size_t index=1; index<group->remaining; ++index)
        state.m_pending_parts.push_back({group->request->m_control,
            [&state,group,index](auto& worker, auto&& easy)
            { submit_part(state,worker,std::move(easy),group,index); }});
    google_easy_t first{curl_easy_t{group->request->take_transport()}};
    submit_part(state,multi,std::move(first),group,0);
}

auto task_part_begin(load_state_t& state, curl_multi_worker_t& multi, curl_easy_t&& easy) -> bool
{
    for (auto entry=state.m_pending_parts.begin(); entry!=state.m_pending_parts.end();)
    {
        const auto control = entry->control;
        if (control->cancelled) { entry=state.m_pending_parts.erase(entry); continue; }
        if (control->paused || control->user_paused) { ++entry; continue; }
        auto submit = std::move(entry->submit);
        state.m_pending_parts.erase(entry);
        submit(multi,std::move(easy));
        return true;
    }
    return false;
}
