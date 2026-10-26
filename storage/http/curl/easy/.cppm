export module storage.http.curl.easy;

import std;

import external.curl;

export
{
    class curl_easy_t
    {
    public:
        static auto get() -> curl_easy_t&;

        struct read_t
        {
            std::string m_payload{};
            std::uint64_t m_progress{};
        };

        struct write_t
        {
            std::string m_payload{};
            std::uint64_t m_progress{};
        };

        curl_easy_t();
        //Explicit because the destructor of this class calls curl_easy_cleanup.
        explicit curl_easy_t(CURL* in);
        curl_easy_t(const curl_easy_t&) = delete;
        curl_easy_t(curl_easy_t&& in);
        auto operator=(curl_easy_t&& in) -> curl_easy_t&;
        virtual ~curl_easy_t();
        auto free() -> void;
        //Make this a null handle without calling curl_easy_cleanup.
        auto release() -> void;

        auto interface_get() const -> CURL*;
        operator CURL*() const;
        auto operator->() const -> CURL*;

        auto nonempty() const -> bool;
        operator bool() const;

        virtual auto clear() -> void;
        auto host_get() const -> std::string;
        auto host_set(std::string host) -> void;
        auto path_get() const -> std::string;
        auto path_set(std::string path) -> void;
        auto query_get(std::string key) const -> std::string;
        auto query_set(std::string key, std::string val) -> void;
        auto header_get(std::string name) const -> std::string;
        auto header_set(std::string name, std::string val) -> void;

        auto method_get() -> void;
        auto method_post(std::string payload) -> void;
        auto method_patch(std::string payload) -> void;

        auto run() -> void;
        auto run_try() -> void;
        // Nonblocking preparation before each attempt. False keeps the request
        // outside CURLM while asynchronous credentials are being refreshed.
        virtual auto prepare() -> bool;
        //Call parse() first then valid().
        virtual auto parse() -> void;
        //valid() is meaningful only after a call to parse().
        virtual auto valid() -> bool;
        virtual auto resume() -> void;

        auto payload() -> std::string;
    protected:
        static auto debug_callback(CURL*, curl_infotype type, char* data, std::size_t size, void*) -> int;
        static auto read_callback(char* dest, std::size_t, std::size_t size, void* src_void) -> std::size_t;
        static auto seek_callback(void* read_void, curl_off_t offset, int origin) -> int;
        static auto write_callback(char* src, std::size_t, std::size_t size, void* dest_void) -> std::size_t;

        CURL* m_interface{};
        std::string m_host{};
        std::string m_path{};
        std::map<std::string, std::string> m_query{};
        std::map<std::string, std::string> m_header{};
        //Use a unique_ptr to ensure that the address is unchanged under move.
        std::unique_ptr<read_t> m_read{};
        //Use a unique_ptr to ensure that the address is unchanged under move.
        std::unique_ptr<write_t> m_write{};
        long m_response_code{};
        curl_slist* m_header_curl{};

        //Finalizes URL and headers.
        virtual auto method() -> void;

        //Do NOT write `friend class curl_multi_t;` here.
        //In standard C++23 modules (enforced by Clang), an unqualified friend declaration
        //`friend class curl_multi_t;` forward-declares `curl_multi_t` as belonging to module
        //`storage.http.curl.easy`, which conflicts with its actual definition in downstream module
        //`storage.http.curl.multi`. Instead, expose a helper struct in this module for `curl_multi_t`.
        friend struct curl_easy_multi_access_t;
    };
    struct curl_easy_multi_access_t
    {
        static auto write_get(curl_easy_t& e) -> std::unique_ptr<curl_easy_t::write_t>& { return e.m_write; }
        static auto read_get(curl_easy_t& e) -> std::unique_ptr<curl_easy_t::read_t>& { return e.m_read; }
        static auto interface_get(curl_easy_t& e) -> CURL*& { return e.m_interface; }
        static auto response_code_get(curl_easy_t& e) -> long& { return e.m_response_code; }
    };
}
