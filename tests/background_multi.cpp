#include <cstdio>

import std;
import external.curl;
import external.GLFW;
import lgo.io;
import lgo.math;
import lgo.GUI;
import storage.http;

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

int main(int argc, char** argv)
{
    std::set_terminate([] { std::_Exit(42); });
    try
    {
        check(argc == 3, "Expected mode and local fixture URL.");
        curl_t curl;
        lgo::init_logfile("logfile/background-multi.txt");
        if (std::string_view{ argv[1] } == "ownership") ownership_test(argv[2]);
        else stalled_test(argv[2]);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::println("FAIL: {}", error.what());
        return 1;
    }
}
