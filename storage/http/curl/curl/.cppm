export module storage.http.curl.curl;

import std;

import external.curl;

export
{
    auto api_curl(CURLcode code) -> void;
    auto api_curlh(CURLHcode code) -> void;

    class curl_t
    {
    public:
        curl_t();
        ~curl_t();
    };
}
