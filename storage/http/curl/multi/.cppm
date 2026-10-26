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
        static auto get() -> curl_multi_t&;

        curl_multi_t();
        curl_multi_t(const curl_multi_t&) = delete;
        ~curl_multi_t();

        auto interface_get() const -> CURLM*;
        operator CURLM*() const;
        auto operator->() const -> CURLM*;

        template<typename t_easy>
        auto add(t_easy&& easy) -> void;
        //Returns the number of failed requests.
        auto run() -> std::uint64_t;
        auto running_count() -> int;
        auto read_progress(CURL* key) -> std::uint64_t;
        //Block until one of the handles has activity, or the timeout set or the internal timeout expires.
        auto poll(int timeout_ms) -> void;

        auto ready_nonempty() -> bool;
        //Returns the added t_easy with the response.
        template<typename t_easy>
        auto ready_get() -> t_easy;
        //Returns the added curl_easy_t with the response.
        auto ready_get_unique() -> std::unique_ptr<curl_easy_t>;
    private:
        CURLM* m_interface{};
        int m_running_count{};
        std::map<CURL*, std::unique_ptr<curl_easy_t>> m_added{};
        std::set<CURL*> m_ready{};
    };
}

template<typename t_easy>
auto curl_multi_t::add(t_easy&& easy) -> void
{
    curl_easy_multi_access_t::write_get(easy) = std::make_unique<curl_easy_t::write_t>();
    api_curl(curl_easy_setopt(curl_easy_multi_access_t::interface_get(easy), CURLOPT_WRITEDATA, static_cast<void*>(curl_easy_multi_access_t::write_get(easy).get())));
    api_curlm(curl_multi_add_handle(m_interface, easy.interface_get()));
    ++m_running_count;
    //Get the key to `m_added` first before `easy` is moved.
    CURL* key{ easy.interface_get() };
    m_added[key] = std::make_unique<t_easy>(std::move(easy));
    return;
}

template<typename t_easy>
auto curl_multi_t::ready_get() -> t_easy
{
    auto easy{ ready_get_unique() };
    return std::move(dynamic_cast<t_easy&>(*easy));
}

auto curl_multi_t::ready_get_unique() -> std::unique_ptr<curl_easy_t>
{
    if (m_ready.empty())
    {
        throw;
    }
    CURL* ptr{ *m_ready.begin() };
    m_ready.erase(ptr);
    auto out_ptr{ std::move(m_added.at(ptr)) };
    // Returned handles may be destroyed or submitted to a different multi.
    m_added.erase(ptr);
    return out_ptr;
}
