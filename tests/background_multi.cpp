#include <cstdio>

import std;
import external.curl;
import external.GLFW;
import lgo.io;
import lgo.math;
import lgo.GUI;
import storage.http;
import storage.kernel.load;

using namespace std::chrono_literals;

auto check(bool condition, const char* message) -> void
{
    if (!condition) throw std::runtime_error{ message };
}

struct probe_t : curl_easy_t
{
    std::thread::id parsed_on{};
    bool fail_parse{};

    probe_t(const std::string& url, bool fail = false) : fail_parse{ fail }
    {
        host_set(url);
        method_get();
        api_curl(curl_easy_setopt(interface_get(), CURLOPT_VERBOSE, 0L));
        api_curl(curl_easy_setopt(interface_get(), CURLOPT_PROXY, ""));
    }
    auto parse() -> void override
    {
        parsed_on = std::this_thread::get_id();
        if (fail_parse) throw std::runtime_error{ "injected parse failure" };
    }
};

auto receive(curl_multi_worker_t& worker) -> std::unique_ptr<curl_easy_t>
{
    const auto deadline{ std::chrono::steady_clock::now() + 10s };
    while (!worker.ready_nonempty())
    {
        worker.run();
        check(std::chrono::steady_clock::now() < deadline, "Completion timed out.");
        std::this_thread::sleep_for(1ms);
    }
    return worker.ready_get_unique();
}

auto ownership_test(const std::string& url) -> void
{
    curl_multi_worker_t second;
    std::unique_ptr<curl_easy_t> returned;
    {
        curl_multi_worker_t first;
        first.add(probe_t{ url });
        returned = receive(first);
        auto& probe{ dynamic_cast<probe_t&>(*returned) };
        check(probe.parsed_on != std::this_thread::get_id(), "Response parsed on the GUI thread.");
        check(probe.payload() == "background-ok", "Unexpected response bytes.");
        // Move the actual handle to another worker before destroying its old owner.
        probe.method_get();
        second.add(std::move(probe));
    }
    returned = receive(second);
    check(returned->payload() == "background-ok", "Handle reuse lost its response.");
    second.stop();
    second.stop();

    curl_multi_worker_t errors;
    errors.add(probe_t{ url, true });
    bool propagated{};
    auto deadline{ std::chrono::steady_clock::now() + 10s };
    while (!propagated && std::chrono::steady_clock::now() < deadline)
    {
        try { errors.run(); }
        catch (const std::runtime_error& error)
        {
            propagated = std::string_view{ error.what() } == "injected parse failure";
        }
        std::this_thread::sleep_for(1ms);
    }
    check(propagated, "Worker exception was not propagated.");
    errors.stop();
    for (int i{}; i < 100; ++i) { curl_multi_worker_t idle; }
    std::println("PASS: background parsing, ownership across workers, exact bytes, exception propagation and idle shutdown");
}

std::atomic_size_t received_bytes{};
auto record_download(char* bytes, std::size_t size, std::size_t count, void* context) -> std::size_t
{
    auto& output{*static_cast<curl_easy_t::write_t*>(context)};
    output.m_payload.append(bytes, size * count);
    received_bytes += size * count;
    return size * count;
}

auto pause_test(const std::string& url) -> void
{
    auto wait = [](auto predicate, const char* message)
    {
        const auto deadline{std::chrono::steady_clock::now() + 10s};
        while (!predicate())
        {
            check(std::chrono::steady_clock::now() < deadline, message);
            std::this_thread::sleep_for(5ms);
        }
    };
    // Exercise real in-flight I/O, not just a scheduler waiting to submit.
    for (bool synchronous : {false, true})
    {
        received_bytes = 0;
        auto control{std::make_shared<request_control_t>()};
        control->retry_limit = 0;
        probe_t request{url + "pause-download"};
        request.control_set(control);
        api_curl(curl_easy_setopt(request.interface_get(), CURLOPT_WRITEFUNCTION, &record_download));
        curl_multi_worker_t worker;
        std::future<void> sync;
        if (synchronous)
            sync = std::async(std::launch::async, [&] { request.run(); });
        else
            worker.add(std::move(request));
        wait([] { return received_bytes > 0; }, "Download never started.");
        control->pause(true);
        std::this_thread::sleep_for(150ms); // Allow the owner to observe the flag.
        const auto stopped{received_bytes.load()};
        check(stopped < 524288, "Download finished before pause was applied.");
        std::this_thread::sleep_for(400ms);
        check(received_bytes == stopped, "Paused download continued receiving application bytes.");
        std::unique_ptr<curl_easy_t> parked;
        if (!synchronous)
        {
            parked = receive(worker);
            check(parked->m_suspended && !worker.pending(control), "Pause retained an active request/slot.");
            worker.add(probe_t{url});
            check(receive(worker)->payload() == "background-ok", "Paused task blocked another task.");
        }
        const auto epoch{control->epoch.load()};
        control->pause(false);
        if (synchronous)
        {
            wait([&] { return sync.wait_for(0s) == std::future_status::ready; }, "Sync resume stalled.");
            sync.get();
            check(request.payload() == std::string(524288, 'd'), "Sync pause duplicated/lost bytes.");
        }
        else
        {
            worker.add(std::move(parked));
            check(receive(worker)->payload() == std::string(524288, 'd'), "Pause duplicated/lost bytes.");
        }
        check(!control->paused && control->epoch == epoch, "Resume consumed/reset the network retry budget.");
        if (synchronous)
        {
            auto reuse{std::async(std::launch::async, [&]
            {
                for (unsigned i{}; i < 5; ++i)
                {
                    // Shutdown an idle cached connection and resume before the
                    // next request: it must not become a retry-limit-zero error.
                    control->pause(true); control->pause(false);
                    request.host_set(url); request.method_get(); request.run();
                    check(request.payload() == "background-ok", "Sync cache reuse lost its response.");
                }
            })};
            if (reuse.wait_for(5s) != std::future_status::ready)
            {
                control->cancel();
                throw std::runtime_error{"Sync Pause/Continue reused a shutdown connection."};
            }
            reuse.get();
        }
        std::println("PASS: {} download pause/resume", synchronous ? "synchronous" : "background");
        std::fflush(stdout);
    }
    // An unfinished upload is aborted and rewound before resubmission.
    curl_multi_worker_t worker;
    auto control{std::make_shared<request_control_t>()};
    control->retry_limit = 0;
    probe_t upload{url + "pause-upload"};
    upload.method_post(std::string(524288, 'u'));
    upload.control_set(control);
    auto* key{upload.interface_get()};
    api_curl(curl_easy_setopt(key, CURLOPT_MAX_SEND_SPEED_LARGE, curl_off_t{131072}));
    worker.add(std::move(upload));
    wait([&] { worker.run(); return worker.read_progress(key) > 0; }, "Upload never started.");
    control->pause(true);
    std::this_thread::sleep_for(200ms);
    const auto stopped{worker.read_progress(key)};
    check(stopped < 524288, "Upload completed before pause.");
    std::this_thread::sleep_for(400ms);
    check(worker.read_progress(key) == stopped, "Paused upload continued reading its body.");
    auto parked_upload{receive(worker)};
    check(parked_upload->m_suspended && !worker.pending(control), "Upload pause held its slot.");
    control->pause(false);
    worker.add(std::move(parked_upload));
    check(receive(worker)->payload() == "upload-ok", "Resumed upload changed its body.");
    check(!control->paused && control->epoch == 0, "Upload pause changed retry state.");
    std::println("PASS: background upload pause/resume");
    std::fflush(stdout);

    // Cancellation while paused must detach and drain the easy handle.
    received_bytes = 0;
    auto cancelled{std::make_shared<request_control_t>()};
    probe_t download{url + "pause-download"};
    download.control_set(cancelled);
    api_curl(curl_easy_setopt(download.interface_get(), CURLOPT_WRITEFUNCTION, &record_download));
    worker.add(std::move(download));
    wait([] { return received_bytes > 0; }, "Cancel download never started.");
    cancelled->pause(true);
    std::this_thread::sleep_for(100ms);
    cancelled->cancel();
    auto drained{receive(worker)};
    check(drained->cancelled(), "Paused request did not cancel.");
    check(!worker.pending(cancelled), "Cancelled request was retained.");
    drained->control_set({});
    drained->host_set(url);
    drained->method_get();
    worker.add(std::move(dynamic_cast<probe_t&>(*drained)));
    check(receive(worker)->payload() == "background-ok", "Reused cancelled handle retained its pause flags.");

    // A response completed before Pause is returned as successful, so the GUI
    // can park its logical result and immediately reclaim the physical slot.
    auto held{std::make_shared<request_control_t>()};
    probe_t completed{url};
    completed.control_set(held);
    worker.add(std::move(completed));
    wait([&] { worker.run(); return worker.ready_nonempty(); }, "Held response never finished.");
    held->pause(true);
    auto success{receive(worker)};
    check(!success->m_suspended && success->payload() == "background-ok", "Pause discarded a successful response.");
    std::println("PASS: abort/reissue upload/download, exact bytes, synchronous preparation, retry isolation and cancellation");

}

// Use the production scheduler handoff, with exactly 64 physical handles.
// Creating a 65th handle would hide the old slot starvation bug.
auto pool_pause_test(const std::string& url) -> void
{
    curl_multi_worker_t worker;
    load_state_t state;
    auto control{std::make_shared<request_control_t>()};
    control->retry_limit = 0;
    unsigned completed{};
    received_bytes = 0;
    for (unsigned i{}; i < 64; ++i)
    {
        probe_t request{url + "pause-download"};
        request.control_set(control);
        api_curl(curl_easy_setopt(request.interface_get(), CURLOPT_WRITEFUNCTION, &record_download));
        state.m_end.emplace(request.interface_get(), [&](auto& result)
        {
            check(result->payload() == std::string(524288, 'd'), "Pool resume lost download bytes.");
            ++completed;
        });
        worker.add(std::move(request));
    }
    const auto deadline{std::chrono::steady_clock::now() + 20s};
    while (!received_bytes)
    {
        check(std::chrono::steady_clock::now() < deadline, "Full pool never started.");
        worker.run();
        std::this_thread::sleep_for(5ms);
    }
    control->pause(true);
    std::vector<std::unique_ptr<curl_easy_t>> pool;
    while (pool.size() < 64)
    {
        check(std::chrono::steady_clock::now() < deadline, "Pause did not release all 64 slots.");
        auto result{receive(worker)};
        transfer_end(&state, result);
        pool.push_back(std::move(result));
    }
    check(!worker.pending(control) && completed == 0 && state.m_end.empty(), "Paused callbacks ran or slots remained busy.");
    check(state.m_parked.size() == 64, "Suspended requests were lost.");
    {
        std::lock_guard lock{control->mutex};
        check(control->sockets.empty(), "Paused task retained an open connection.");
    }
    for (const auto& parked : state.m_parked)
        check(!parked.request->nonempty(), "Parked request retained its physical curl handle.");

    // Use one reclaimed slot for a different task with no network retries.
    auto other{std::make_shared<request_control_t>()};
    other->retry_limit = 0;
    auto probe{std::move(pool.back())}; pool.pop_back();
    probe->control_set(other);
    probe->host_set(url);
    probe->method_get();
    // Plain curl request avoids invoking Google OAuth in this local fixture.
    curl_easy_t plain{std::move(*probe)};
    worker.add(std::move(plain));
    auto result{receive(worker)};
    check(result->payload() == "background-ok" && !other->paused && control->user_paused,
          "Another task could not finish using the paused task's slot.");
    pool.push_back(std::move(result));

    // Continue, pause/continue immediately, and then finish every callback once.
    control->pause(false);
    for (unsigned cycle{}; cycle < 3; ++cycle)
    {
        check(transfer_begin(&state, worker, std::move(*pool.back()), nullptr), "Parked request was not rescheduled.");
        pool.pop_back();
        control->pause(true);
        control->pause(false);
        auto retry{receive(worker)};
        transfer_end(&state, retry);
        pool.push_back(std::move(retry));
    }
    while (completed < 64)
    {
        check(std::chrono::steady_clock::now() < deadline, "Pool resume stalled.");
        if (!pool.empty() && transfer_begin(&state, worker, std::move(*pool.back()), nullptr)) pool.pop_back();
        while (worker.ready_nonempty())
        {
            auto ready{worker.ready_get_unique()};
            transfer_end(&state, ready);
            pool.push_back(std::move(ready));
        }
        worker.run();
        std::this_thread::sleep_for(5ms);
    }
    check(pool.size() == 64 && state.m_parked.empty() && state.m_end.empty(), "Pool/callback accounting changed.");
    check(!control->paused && control->epoch == 0, "Pause consumed a zero network retry budget.");
    // A successful result racing with Pause must never be requested again.
    auto success{std::move(pool.back())}; pool.pop_back();
    success->control_set(control);
    success->host_set(url); success->method_get();
    auto* key{success->interface_get()};
    state.m_end.emplace(key, [&](auto& value)
        { check(value->payload() == "background-ok", "Parked success changed."); ++completed; });
    worker.add(std::move(success));
    success = receive(worker);
    control->pause(true);
    transfer_end(&state, success);
    check(completed == 64 && state.m_parked.size() == 1, "Paused success callback ran early.");
    control->fail("Injected independent network failure.");
    control->retry();
    control->pause(false);
    check(!transfer_begin(&state, worker, std::move(*success), nullptr) && completed == 65,
          "Completed paused response was reissued or lost.");
    for (unsigned i{}; i < 10; ++i)
    {
        control->pause(true); control->pause(false);
        probe_t immediate{url}; immediate.control_set(control);
        worker.add(std::move(immediate));
        check(receive(worker)->payload() == "background-ok" && !control->paused,
              "Rapid Pause/Continue reused a shutdown task cache.");
    }
    auto failed{std::make_shared<download_task_t>()};
    failed->m_control = control; failed->m_type = "download";
    state.m_task_list.push_back(failed);
    // Error handling must also cover requests parked before another response
    // exposed a local decoding/writing error in the same download.
    probe_t failed_probe{url}; failed_probe.control_set(control);
    state.m_end.emplace(failed_probe.interface_get(), [&](auto&) { ++completed; });
    worker.add(std::move(failed_probe));
    auto failed_result{receive(worker)};
    control->pause(true);
    transfer_end(&state, failed_result);
    failed->m_error = "Injected local output error.";
    control->pause(false);
    check(!transfer_begin(&state, worker, std::move(*failed_result), nullptr) &&
          state.m_parked.empty() && completed == 65 && !worker.pending(control),
          "Failed download resubmitted its parked requests or invoked stale callbacks.");
    std::println("PASS: production 64-slot reclamation, other-task completion before Continue, rapid pause/resume, one-shot callbacks");
}

std::atomic_size_t other_received{};
auto record_other(char* bytes, std::size_t size, std::size_t count, void* context) -> std::size_t
{
    static_cast<curl_easy_t::write_t*>(context)->m_payload.append(bytes, size * count);
    other_received += size * count;
    return size * count;
}

auto pause_isolation_test(const std::string& url) -> void
{
    curl_multi_worker_t worker;
    auto first{std::make_shared<request_control_t>()}; first->retry_limit = 0;
    auto other{std::make_shared<request_control_t>()}; other->retry_limit = 0;
    received_bytes = 0; other_received = 0;
    probe_t a{url + "pause-download"}; a.control_set(first);
    probe_t b{url + "pause-download"}; b.control_set(other);
    api_curl(curl_easy_setopt(a.interface_get(), CURLOPT_WRITEFUNCTION, &record_download));
    api_curl(curl_easy_setopt(b.interface_get(), CURLOPT_WRITEFUNCTION, &record_other));
    worker.add(std::move(a)); worker.add(std::move(b));
    const auto deadline{std::chrono::steady_clock::now() + 10s};
    while (!received_bytes || !other_received)
    {
        worker.run();
        check(std::chrono::steady_clock::now() < deadline, "Concurrent transfers did not start.");
        std::this_thread::sleep_for(5ms);
    }
    first->pause(true);
    auto parked{receive(worker)};
    check(parked->m_control == first && parked->m_suspended, "Paused request was not detached.");
    auto finished{receive(worker)};
    check(finished->m_control == other && finished->payload() == std::string(524288, 'd') &&
          !other->paused && other->epoch == 0 && first->user_paused,
          "Pausing a task aborted a different task or consumed its retry budget.");
    first->cancel();
    worker.add(std::move(parked));
    check(receive(worker)->cancelled(), "Cancelling a detached request stalled.");
    std::println("PASS: pausing one in-flight task leaves another in-flight task intact with zero retries");
}

std::atomic<std::uint64_t> sent_bytes{};
auto stream(char* data, std::size_t size, std::size_t count, void*) -> std::size_t
{
    auto bytes{ size * count };
    std::memset(data, 'x', bytes);
    sent_bytes += bytes;
    return bytes;
}

auto stalled_test(const std::string& url) -> void
{
    lgo::engine_t engine{ lgo::size_2D{ 800, 600 }, "Background multi offline test" };
    curl_multi_worker_t worker;
    std::vector<CURL*> keys;
    for (int i{}; i < 64; ++i)
    {
        curl_easy_t easy;
        easy.host_set(url);
        easy.method_post("");
        auto* key{ easy.interface_get() };
        keys.push_back(key);
        api_curl(curl_easy_setopt(key, CURLOPT_VERBOSE, 0L));
        api_curl(curl_easy_setopt(key, CURLOPT_PROXY, ""));
        // Self-signed certificate belongs only to this loopback fixture.
        api_curl(curl_easy_setopt(key, CURLOPT_SSL_VERIFYPEER, 0L));
        api_curl(curl_easy_setopt(key, CURLOPT_SSL_VERIFYHOST, 0L));
        api_curl(curl_easy_setopt(key, CURLOPT_POSTFIELDSIZE_LARGE, curl_off_t{ 1073741824 }));
        api_curl(curl_easy_setopt(key, CURLOPT_READFUNCTION, &stream));
        worker.add(std::move(easy));
    }
    std::println("READY: 64 uploads, production 300 second timeout");
    std::fflush(stdout);
    const auto start{ std::chrono::steady_clock::now() };
    auto last_resize{ start };
    std::size_t frames{};
    std::size_t resizes{};
    double max_poll_ms{};
    while (std::chrono::steady_clock::now() - start < 6s)
    {
        auto before{ std::chrono::steady_clock::now() };
        worker.run();
        for (auto* key : keys) worker.read_progress(key);
        worker.ready_nonempty();
        max_poll_ms = std::max(max_poll_ms,
            std::chrono::duration<double, std::milli>{ std::chrono::steady_clock::now() - before }.count());
        if (before - last_resize > 500ms)
        {
            ++resizes;
            glfwSetWindowSize(engine.window_get(), 800 + (resizes % 2) * 100, 600);
            last_resize = before;
        }
        check(!engine.message_loop(), "Window closed during stalled transfers.");
        ++frames;
        std::this_thread::sleep_for(1ms);
    }
    check(sent_bytes > 65536, "Uploads never started.");
    check(frames > 30 && resizes > 5, "Rendering or resize events stalled.");
    check(max_poll_ms < 250, "GUI polling waited for network work.");
    auto before{ std::chrono::steady_clock::now() };
    worker.stop();
    auto stop_ms{ std::chrono::duration<double, std::milli>{ std::chrono::steady_clock::now() - before }.count() };
    check(stop_ms < 2000, "Shutdown did not interrupt the blocked socket wait.");
    engine.flush();
    std::println("PASS: frames={} resizes={} bytes={} max_poll_ms={:.3f} stop_ms={:.3f}",
        frames, resizes, sent_bytes.load(), max_poll_ms, stop_ms);
}

auto stalled_pause_test(const std::string& url) -> void
{
    curl_multi_worker_t worker;
    auto control{std::make_shared<request_control_t>()};
    control->retry_limit = 0;
    sent_bytes = 0;
    for (unsigned i{}; i < 64; ++i)
    {
        curl_easy_t easy;
        easy.host_set(url); easy.method_post(""); easy.control_set(control);
        api_curl(curl_easy_setopt(easy.interface_get(), CURLOPT_VERBOSE, 0L));
        api_curl(curl_easy_setopt(easy.interface_get(), CURLOPT_PROXY, ""));
        api_curl(curl_easy_setopt(easy.interface_get(), CURLOPT_SSL_VERIFYPEER, 0L));
        api_curl(curl_easy_setopt(easy.interface_get(), CURLOPT_SSL_VERIFYHOST, 0L));
        api_curl(curl_easy_setopt(easy.interface_get(), CURLOPT_POSTFIELDSIZE_LARGE, curl_off_t{1073741824}));
        api_curl(curl_easy_setopt(easy.interface_get(), CURLOPT_READFUNCTION, &stream));
        worker.add(std::move(easy));
    }
    const auto deadline{std::chrono::steady_clock::now() + 10s};
    while (sent_bytes < 65536)
    {
        worker.run();
        check(std::chrono::steady_clock::now() < deadline, "Pause TLS upload never started.");
        std::this_thread::sleep_for(5ms);
    }
    std::this_thread::sleep_for(1500ms); // Fill TLS/socket buffers until Schannel waits.
    const auto before{std::chrono::steady_clock::now()};
    control->pause(true);
    for (unsigned i{}; i < 64; ++i)
    {
        auto result{receive(worker)};
        check(result->m_suspended, "TLS pause was reported as a network failure.");
    }
    const auto pause_ms{std::chrono::duration<double, std::milli>{std::chrono::steady_clock::now() - before}.count()};
    check(pause_ms < 2000 && !worker.pending(control), "Pause did not wake Schannel and detach all requests.");
    check(!control->paused && control->epoch == 0, "TLS pause spent the network retry budget.");
    auto other{std::make_shared<request_control_t>()}; other->retry_limit = 0;
    probe_t probe{url + "probe"}; probe.control_set(other);
    api_curl(curl_easy_setopt(probe.interface_get(), CURLOPT_SSL_VERIFYPEER, 0L));
    api_curl(curl_easy_setopt(probe.interface_get(), CURLOPT_SSL_VERIFYHOST, 0L));
    worker.add(std::move(probe));
    check(receive(worker)->payload() == "background-ok" && !other->paused && control->user_paused,
          "TLS pause prevented another task from completing.");
    std::println("PASS: blocked Schannel pause released 64 requests in {:.3f} ms; another task completed before Continue", pause_ms);
}

int main(int argc, char** argv)
{
    std::set_terminate([] { std::_Exit(42); });
    try
    {
        check(argc == 3, "Expected mode and local fixture URL.");
        curl_t curl;
        lgo::init_logfile("logfile/background-multi.txt");
        if (std::string_view{ argv[1] } == "ownership") ownership_test(argv[2]);
        else if (std::string_view{ argv[1] } == "pause") { pause_test(argv[2]); pool_pause_test(argv[2]); pause_isolation_test(argv[2]); }
        else
        {
            stalled_test(argv[2]);
            if (std::string_view{argv[1]} == "stalled-pause") stalled_pause_test(argv[2]);
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::println("FAIL: {}", error.what());
        return 1;
    }
}
