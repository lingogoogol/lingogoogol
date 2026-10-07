export module storage.http.curl.multi;

import std;

import external.curl;

import storage.http.curl.easy;

export
{
    auto api_curlm(CURLMcode code) -> void;

    class curl_multi_t
    {
    public:
        static auto get() -> const curl_multi_t&;

        curl_multi_t();
        curl_multi_t(const curl_multi_t&) = delete;
        ~curl_multi_t();

        auto interface_get() const -> CURLM*;
        operator CURLM*() const;
        auto operator->() const -> CURLM*;

        auto add(curl_easy_t&& easy) -> void;
        auto run() -> int;
        auto running_count() -> int;
        //Block until one of the handles has activity, or the timeout set or the internal timeout expires.
        auto poll(int timeout_ms) -> void;
        auto available() -> curl_easy_t;
    private:
        CURLM* m_interface{};
        int m_running_count{};
        std::set<CURL*> m_available{};
    };
}
