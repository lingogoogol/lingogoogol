module storage.google.easy;
import std;
import external.curl;
import storage.http.curl.easy;
namespace {
    std::mutex owner_mutex;
    std::map<google_easy_t*, std::thread::id> thread_local_owners;
    auto mock_host() -> std::string {
        const char* port{ std::getenv("STORAGE_GPU_TEST_PORT") };
        if (!port) throw std::runtime_error{ "Missing mock server port." };
        return std::string{ "http://127.0.0.1:" } + port;
    }
}
auto google_easy_t::get() -> google_easy_t& {
    thread_local google_easy_t easy{};
    std::unique_lock lock{ owner_mutex };
    thread_local_owners[&easy] = std::this_thread::get_id();
    return easy;
}
google_easy_t::google_easy_t() : curl_easy_t{} { host_set(mock_host()); }
google_easy_t::google_easy_t(curl_easy_t&& in) : curl_easy_t{std::move(in)} { host_set(mock_host()); }
auto google_easy_t::clear() -> void {
    {
        std::unique_lock lock{ owner_mutex };
        auto owner{ thread_local_owners.find(this) };
        if (owner != thread_local_owners.end() && owner->second != std::this_thread::get_id())
            throw std::runtime_error{ "Preparation reused another thread's HTTP handle." };
    }
    curl_easy_t::clear(); host_set(mock_host());
}
auto google_easy_t::method() -> void { curl_easy_setopt(interface_get(), CURLOPT_VERBOSE, 0L); curl_easy_t::method(); }
