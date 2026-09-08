module;

#include <curl/curl.h>

export module external.curl;

constexpr auto CURLH_HEADER_export{ CURLH_HEADER };
#undef CURLH_HEADER

export {
    using ::CURL;
    using ::curl_slist;
    using ::curl_header;
    using ::curl_off_t;

    using ::CURLcode;
    using enum ::CURLcode;
    using ::CURLHcode;
    using enum ::CURLHcode;
    using ::curl_infotype;
    using enum ::curl_infotype;
    using ::CURLoption;
    using enum ::CURLoption;

    using ::curl_easy_init;
    using ::curl_easy_cleanup;
    using ::curl_easy_strerror;
    using ::curl_easy_setopt;
    using ::curl_easy_perform;
    using ::curl_easy_header;

    using ::curl_slist_append;
    using ::curl_slist_free_all;
    
    constexpr auto CURLH_HEADER{ CURLH_HEADER_export };
}
