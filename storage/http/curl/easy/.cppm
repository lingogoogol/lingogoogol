export module storage.http.curl.easy;

import std;

import external.curl;

export
{
    class curl_easy_t
    {
    public:
        struct read_src_t
        {
            std::string m_data{};
            std::uint64_t m_progress{};
        };

        static auto get() -> curl_easy_t&;

        curl_easy_t();
        //Explicit because the destructor of this class calls curl_easy_cleanup.
        explicit curl_easy_t(CURL* in);
        curl_easy_t(const curl_easy_t&) = delete;
        curl_easy_t(curl_easy_t&& in);
        auto operator=(curl_easy_t&& in) -> curl_easy_t&;
        ~curl_easy_t();
        auto free() -> void;
        //Make this a null handle without calling curl_easy_cleanup.
        auto release() -> void;

        auto interface_get() const -> CURL*;
        operator CURL*() const;
        auto operator->() const -> CURL*;

        auto nonempty() const -> bool;
        operator bool() const;

        auto host_get() const -> std::string;
        auto host_set(std::string host) -> void;
        auto path_get() const -> std::string;
        auto path_set(std::string path) -> void;
        auto query_get(std::string key) const -> std::string;
        auto query_set(std::string key, std::string val) -> void;
        auto header_get(std::string name) const -> std::string;
        auto header_set(std::string name, std::string val) -> void;

        auto method_post(std::string body) -> void;
        auto method_post_run(std::string body) -> std::string;
        auto method_post_run_try(std::string body) -> std::string;
    private:
        static auto debug_callback(CURL*, curl_infotype type, char* data, std::size_t size, void*) -> int;
        static auto read_callback(char* dest, std::size_t, std::size_t size, void* src_void) -> std::size_t;
        static auto write_callback(char* src, std::size_t, std::size_t size, void* dest_void) -> std::size_t;

        CURL* m_interface{};
        std::string m_path{};
        std::string m_host{};
        std::map<std::string, std::string> m_query{};
        std::map<std::string, std::string> m_header{};
        curl_slist* m_header_curl{};
    };
}
