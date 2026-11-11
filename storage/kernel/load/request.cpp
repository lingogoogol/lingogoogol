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
                     const std::shared_ptr<request_group_t>& group, std::size_t index,
                     std::size_t count, bool batched) -> void
    {
        google_batch_t request{std::move(transport)};
        request.clear();
        request.m_account_context = group->request->m_account_context;
        request.control_set(group->request->m_control);
        request.m_journal = group->request->m_journal;
        request.host_set(group->request->host_get());
        for (std::size_t i{};i<count;++i)
            request.request_get(request.request_add()) = group->request->request_get(index+i);
        if (batched)
        {
            request.google_easy_t::path_set(group->request->path_get());
            request.boundary_set(group->request->boundary_get());
            request.method_post();
        }
        else request.method_single();
        const auto key = request.interface_get();
        state.m_end.emplace(key,[group,index,count](auto& result)
        {
            auto& part = dynamic_cast<google_batch_t&>(*result);
            for (std::size_t i{};i<count;++i)
                group->request->response_get(index+i) = part.response_get(i);
            group->remaining -= count;
            if (group->remaining) return;
            // Keep the same grouped checkpoint and completion contract as batch
            // mode. Each successful envelope has its own durable response, so
            // rebuilding the same envelopes replays completed children.
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
    const bool batched = state.m_settings.batch_request;
    const auto count = batched ? state.m_settings.batch_size : std::size_t{1};
    if ((batched && request.part_count()<=count) || request.replay())
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
    for (std::size_t index=count; index<group->remaining; index+=count)
    {
        const auto size = std::min(count,group->remaining-index);
        state.m_pending_parts.push_back({group->request->m_control,
            [&state,group,index,size,batched](auto& worker, auto&& easy)
            { submit_part(state,worker,std::move(easy),group,index,size,batched); }});
    }
    google_easy_t first{curl_easy_t{group->request->take_transport()}};
    submit_part(state,multi,std::move(first),group,0,std::min(count,group->remaining),batched);
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
