#include <nlohmann/json.hpp>

import std;
import external.curl;
import external.GLFW;
import lgo.io;
import lgo.math;
import lgo.GUI;
import storage.http;
import storage.google;

using namespace std::chrono_literals;

auto check(bool condition, const char* message) -> void
{
    if (!condition) throw std::runtime_error{ message };
}

std::string host;
std::size_t frames{};
auto quiet(curl_easy_t& easy) -> void
{
    api_curl(curl_easy_setopt(easy, CURLOPT_VERBOSE, 0L));
    api_curl(curl_easy_setopt(easy, CURLOPT_PROXY, ""));
}
auto raw(const std::string& path) -> nlohmann::json
{
    curl_easy_t easy;
    easy.host_set(host);
    easy.path_set(path);
    easy.method_get();
    quiet(easy);
    easy.run();
    return nlohmann::json::parse(easy.payload());
}
auto request(const std::string& path, const std::string& method = "GET", const std::string& body = "") -> google_easy_t
{
    google_easy_t easy;
    easy.host_set(host);
    easy.path_set(path);
    if (method == "PATCH") easy.method_patch(body);
    else if (method == "POST") easy.method_post(body);
    else easy.method_get();
    quiet(easy);
    return easy;
}
auto pump(lgo::engine_t& engine) -> void
{
    check(!engine.message_loop(), "GUI closed during OAuth refresh.");
    ++frames;
    if (frames % 40 == 0) glfwSetWindowSize(engine.window_get(), 800 + (frames % 80), 600);
    std::this_thread::sleep_for(1ms);
}
auto receive(curl_multi_worker_t& multi, lgo::engine_t& engine, std::size_t count) -> void
{
    const auto deadline{ std::chrono::steady_clock::now() + 12s };
    while (count)
    {
        multi.run();
        while (multi.ready_nonempty())
        {
            auto response{ multi.ready_get_unique() };
            check(response->payload() == "ok", "Authenticated response/body mismatch.");
            --count;
        }
        check(std::chrono::steady_clock::now() < deadline, "OAuth/Drive transfer stalled.");
        pump(engine);
    }
}

auto recovery(lgo::engine_t& engine) -> void
{
    curl_multi_worker_t multi;
    auto before{ std::chrono::steady_clock::now() };
    for (int i{}; i < 16; ++i) multi.add(request("/warm/" + std::to_string(i)));
    check(std::chrono::steady_clock::now() - before < 250ms, "Request construction blocked on OAuth.");
    receive(multi, engine, 16);
    check(frames > 20, "GUI did not run during initial token retrieval.");
    check(raw("/control/report")["tokens"] == 1, "Initial requests duplicated token refresh.");

    auto queued{ request("/expires", "POST", "queued-before-expiry") };
    const auto expire{ std::chrono::steady_clock::now() + 2200ms };
    while (std::chrono::steady_clock::now() < expire) pump(engine);
    multi.add(std::move(queued));
    receive(multi, engine, 1);
    check(raw("/control/report")["tokens"] == 2, "expires_in was ignored.");

    raw("/control/revoke");
    for (int i{}; i < 16; ++i) multi.add(request("/revoked/" + std::to_string(i), "PATCH", "patch-body-" + std::to_string(i)));
    receive(multi, engine, 16);
    check(raw("/control/report")["tokens"] == 3, "Concurrent or late 401 caused duplicate refreshes.");

    google_batch_t batch;
    batch.host_set(host);
    batch.boundary_set("+");
    batch.path_set("/drive/v3");
    for (const auto* id : { "a", "b" }) blob_get(batch, id, "description");
    batch.method_post();
    quiet(batch);
    multi.add(std::move(batch));
    const auto deadline{ std::chrono::steady_clock::now() + 10s };
    while (!multi.ready_nonempty())
    {
        multi.run();
        check(std::chrono::steady_clock::now() < deadline, "Nested 401 retry stalled.");
        pump(engine);
    }
    auto response{ multi.ready_get_unique() };
    auto& result{ dynamic_cast<google_batch_t&>(*response) };
    check(result.response_get(0).status_code_get() == "200" && result.response_get(1).status_code_get() == "200",
        "Nested 401 did not recover.");
    check(raw("/control/report")["tokens"] == 4, "Nested 401 did not refresh once.");

    multi.add(request("/transient", "POST", "preserve-post-body"));
    receive(multi, engine, 1);
    check(raw("/control/report")["tokens"] == 4, "HTTP 503 unnecessarily refreshed credentials.");
    auto sync{ request("/sync", "PATCH", "preserve-sync-patch") };
    sync.run();
    check(sync.payload() == "ok", "Synchronous retry failed.");
    check(raw("/control/report")["tokens"] == 5, "Synchronous 401 used stale credentials.");

    raw("/control/revoke-sync-multi");
    curl_multi_t sync_multi;
    sync_multi.add(request("/sync-multi", "POST", "sync-multi-body"));
    while (!sync_multi.ready_nonempty())
    {
        sync_multi.run();
        check(std::chrono::steady_clock::now() < deadline, "Synchronous multi retry stalled.");
        pump(engine);
    }
    auto sync_response{ sync_multi.ready_get<google_easy_t>() };
    check(sync_response.payload() == "ok", "Synchronous multi response mismatch.");
    check(sync_response.valid(), "Moving a completed response lost its HTTP status.");
    check(raw("/control/report")["tokens"] == 6, "Synchronous multi did not refresh credentials.");

    curl_multi_worker_t denied;
    denied.add(request("/denied"));
    bool failed{};
    const auto failure_deadline{ std::chrono::steady_clock::now() + 10s };
    while (!failed)
    {
        try { denied.run(); }
        catch (const std::runtime_error& error)
        {
            check(std::string_view{ error.what() }.find("refreshed credentials") != std::string_view::npos,
                "Permanent 401 returned an unexpected error.");
            failed = true;
        }
        check(std::chrono::steady_clock::now() < failure_deadline, "Permanent 401 retried forever.");
        pump(engine);
    }
    denied.stop();
    const auto report = raw("/control/report");
    check(report["tokens"] == 7 && report["denied"] == 2, "Permanent 401 exceeded its refresh budget.");
    check(report["batch_a"] == 1 && report["batch_b"] == 4, "Successful batch parts were resent.");
    check(report["violations"].empty(), "Fixture found missing authorization or changed method/body.");
    multi.stop();
    std::println("PASS: asynchronous initial refresh, expiry, 16 concurrent/late 401s, PATCH/POST rewind, nested batch 401, synchronous easy/multi and permanent 401 limit; GUI frames={}", frames);
}

auto failure_case(lgo::engine_t& engine, bool temporary) -> void
{
    curl_multi_worker_t multi;
    for (int i{}; i < 8; ++i) multi.add(request("/warm/" + std::to_string(i)));
    if (temporary)
    {
        receive(multi, engine, 8);
        check(raw("/control/report")["tokens"] == 3, "Transient OAuth failures were not retried correctly.");
    }
    else
    {
        bool failed{};
        const auto deadline{ std::chrono::steady_clock::now() + 12s };
        while (!failed)
        {
            try { multi.run(); }
            catch (const std::runtime_error& error)
            {
                check(std::string_view{ error.what() }.find("OAuth") != std::string_view::npos, "OAuth failure lost its context.");
                failed = true;
            }
            check(std::chrono::steady_clock::now() < deadline, "OAuth failure was retried indefinitely.");
            pump(engine);
        }
    }
    multi.stop();
    std::println("PASS: OAuth response/transport failure policy; GUI frames={}", frames);
}

auto queue_case(lgo::engine_t& engine, bool shutdown) -> void
{
    std::atomic_bool close{};
    blob_queue_t queue{ "test-queue", &google_account_t::default_get(), &close };
    auto waiter{ std::async(std::launch::async, [&]
    {
        try { queue.get(); }
        catch (const std::exception&) { return true; }
        return false;
    }) };
    const auto deadline{ std::chrono::steady_clock::now() + 10s };
    while (raw("/control/report")["tokens"] == 0)
    {
        check(std::chrono::steady_clock::now() < deadline, "Queue did not start OAuth.");
        pump(engine);
    }
    if (shutdown)
    {
        auto sync_waiter{ std::async(std::launch::async, []
        {
            try { auto easy{ request("/warm/sync") }; easy.run(); }
            catch (const std::exception&) { return true; }
            return false;
        }) };
        auto before{ std::chrono::steady_clock::now() };
        close = true;
        queue.stop();
        google_account_t::default_get().stop();
        check(sync_waiter.wait_for(2s) == std::future_status::ready && sync_waiter.get(), "Synchronous token waiter did not stop.");
        check(std::chrono::steady_clock::now() - before < 2s, "OAuth shutdown waited for request timeout.");
    }
    else
    {
        bool failed{};
        while (!failed)
        {
            try { queue.check_error(); }
            catch (const std::runtime_error&) { failed = true; }
            check(std::chrono::steady_clock::now() < deadline, "Queue swallowed OAuth failure.");
            pump(engine);
        }
    }
    check(waiter.wait_for(2s) == std::future_status::ready && waiter.get(), "Queue waiter was not notified.");
    queue.stop();
    std::println("PASS: background reserve reports OAuth errors and credential/queue waits stop safely");
}

auto batch_headers(lgo::engine_t&) -> void
{
    http_request_t wire;
    wire.method_set("PATCH");
    wire.path_set("/drive/v3/files/test");
    wire.host_set("https://www.googleapis.com");
    wire.header_set("Content-Type", "application/json; charset=UTF-8");
    wire.header_set("X-Fixture", "retained");
    wire.body_set("{}");
    const auto serialized = wire.to_str();
    check(serialized.find("Host: www.googleapis.com\r\n") != std::string::npos &&
          serialized.find("Content-Type: application/json; charset=UTF-8\r\n") != std::string::npos &&
          serialized.find("X-Fixture: retained\r\n") != std::string::npos,
          "Batch serialization dropped headers or used a URL as Host.");
    google_batch_t upload;
    upload.host_set(host);
    upload.boundary_set("fixture");
    upload.path_set("/drive/v3");
    auto upload_control = std::make_shared<request_control_t>();
    upload_control->retry_limit = 0;
    upload.control_set(upload_control);
    blob_upload(upload, "fixture-create", "");
    blob_update(upload, blob_str_opt_t{.m_id="fixture-update", .m_content="fixture-payload"});
    upload.method_post();
    quiet(upload);
    upload.run_try();
    check(upload.valid(), "Strict JSON batch upload rejected missing request headers.");
    check(upload.response_get(1).body_get().find("fixture-payload") != std::string::npos,
          "Batch upload content was lost.");

    std::println("PASS: strict JSON batch uploads, Host and custom headers");
}

auto retry_backoff(lgo::engine_t& engine) -> void
{
    curl_easy_t retry;
    auto unlimited = std::make_shared<request_control_t>();
    unlimited->retry_limit = -1;
    retry.control_set(unlimited);
    for (int i{}; i<3; ++i) retry.failed("fixture retry");
    const auto wait = std::chrono::steady_clock::now() + 2300ms;
    while (std::chrono::steady_clock::now() < wait) pump(engine);
    check(!retry.runnable() && !unlimited->paused, "Unlimited retries failed to increase backoff.");
    unlimited->retry();
    check(retry.runnable(), "Manual retry did not reset backoff.");
    auto finite = std::make_shared<request_control_t>();
    finite->retry_limit = 1;
    curl_easy_t bounded;
    bounded.control_set(finite);
    bounded.failed("first");
    check(!finite->paused, "Retry budget was exhausted too early.");
    bounded.failed("second");
    check(finite->paused && finite->reason()=="second", "Finite retry budget changed.");
    std::println("PASS: unlimited retry backoff, manual reset and finite retry budget");
}

auto batch_errors(lgo::engine_t& engine) -> void
{
    auto failure = [&]
    {
        google_batch_t batch;
        batch.host_set(host);
        batch.boundary_set("fixture");
        batch.path_set("/drive/v3");
        for (const auto* id : {"rate-a", "rate-b", "missing", "private-reason"})
            blob_get(batch, id, "description");
        batch.method_post();
        quiet(batch);
        return batch;
    };
    auto failed = failure();
    failed.run_try();
    check(!failed.valid(), "Outer HTTP 200 hid failed batch subrequests.");
    const auto reason = failed.failure_reason();
    check(reason.find("2 x HTTP 403 (rateLimitExceeded)") != std::string::npos &&
          reason.find("HTTP 404 (notFound)") != std::string::npos &&
          reason.find("HTTP 400") != std::string::npos && reason.find("fixture-token-") == std::string::npos,
          "Batch failure did not safely summarize actual inner statuses/reasons.");
    curl_multi_worker_t worker;
    auto control = std::make_shared<request_control_t>();
    control->retry_limit = 0;
    auto background = failure();
    background.control_set(control);
    worker.add(std::move(background));
    const auto deadline = std::chrono::steady_clock::now() + 8s;
    while (!control->paused)
    {
        worker.run();
        check(std::chrono::steady_clock::now() < deadline, "Failed batch was not reported to its task.");
        pump(engine);
    }
    check(control->reason() == reason, "GUI failure reason lost the batch's inner error.");
    worker.stop();

    std::println("PASS: inner HTTP errors and safe task/log diagnostics");
}

int main(int argc, char** argv)
{
    std::set_terminate([] { std::_Exit(42); });
    try
    {
        check(argc == 3, "Expected test mode and fixture host.");
        host = argv[2];
        const std::string_view mode{ argv[1] };
        curl_t curl;
        lgo::init_logfile("logfile/token-refresh.txt");
        google_account_t::default_get().init("local-test@example.invalid", lgo::get_file_str("fixture.key"));
        lgo::engine_t engine{ lgo::size_2D{ 800, 600 }, "Token refresh regression" };
        if (mode == "batch-headers") batch_headers(engine);
        else if (mode == "retry-backoff") retry_backoff(engine);
        else if (mode == "batch-errors") batch_errors(engine);
        else if (mode == "recovery") recovery(engine);
        else if (mode == "shutdown" || mode == "queue-error") queue_case(engine, mode == "shutdown");
        else failure_case(engine, mode == "temporary");
        google_account_t::default_get().stop();
        engine.flush();
        lgo::log_file("finished_all!\n");
        return 0;
    }
    catch (const std::exception& error)
    {
        google_account_t::default_get().stop();
        std::println("FAIL: {}", error.what());
        return 1;
    }
}
