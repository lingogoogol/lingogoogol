module storage.http.curl.curl;

import std;

import external.curl;

import lgo.dev.error;

auto api_curl(CURLcode code) -> void
{
    if (code != CURLE_OK)
    {
        throw lgo::error_t{ curl_easy_strerror(code) };
    }
    return;
}

auto api_curlh(CURLHcode code) -> void
{
    if (code != CURLHE_OK)
    {
        throw lgo::error_t{ "error curlh: " + std::to_string(code) };
    }
    return;
}

curl_t::curl_t()
{
    api_curl(curl_global_init(CURL_GLOBAL_DEFAULT));
    return;
}

curl_t::~curl_t()
{
	curl_global_cleanup();
    return;
}
