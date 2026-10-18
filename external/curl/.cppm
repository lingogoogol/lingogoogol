module;

#include <curl/curl.h>

export module external.curl;

constexpr auto CURLH_HEADER_export{ CURLH_HEADER };
#undef CURLH_HEADER
constexpr auto CURL_GLOBAL_DEFAULT_export{ CURL_GLOBAL_DEFAULT };
#undef CURL_GLOBAL_DEFAULT

export
{
    using ::CURL;
    using ::CURLM;
    using ::CURLMsg;
    using ::curl_slist;
    using ::curl_header;
    using ::curl_off_t;

    using ::CURLcode;
    using enum ::CURLcode;
    using ::CURLHcode;
    using enum ::CURLHcode;
    using ::CURLMcode;
    using enum ::CURLMcode;
    using ::curl_infotype;
    using enum ::curl_infotype;
    using ::CURLoption;
    using enum ::CURLoption;
    using ::CURLINFO;
    using enum ::CURLINFO;
    using ::CURLMSG;
    using enum ::CURLMSG;

    using ::curl_global_init;
    using ::curl_global_cleanup;

    using ::curl_easy_init;
    using ::curl_easy_cleanup;
    using ::curl_easy_getinfo;
    using ::curl_easy_strerror;
    using ::curl_easy_setopt;
    using ::curl_easy_perform;
    using ::curl_easy_header;

    using ::curl_multi_init;
    using ::curl_multi_cleanup;
    using ::curl_multi_strerror;
    using ::curl_multi_add_handle;
    using ::curl_multi_perform;
    using ::curl_multi_info_read;
    using ::curl_multi_remove_handle;
    using ::curl_multi_poll;

    using ::curl_slist_append;
    using ::curl_slist_free_all;
    
    constexpr auto CURLH_HEADER{ CURLH_HEADER_export };
    constexpr auto CURL_GLOBAL_DEFAULT{ CURL_GLOBAL_DEFAULT_export };
}
