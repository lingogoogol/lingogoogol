module storage.google.easy;

import std;

import storage.google.account;

auto google_easy_t::get() -> google_easy_t&
{
    thread_local google_easy_t easy{};
    return easy;
}

google_easy_t::google_easy_t()
: curl_easy_t{}
{
    host_set("https://www.googleapis.com");
    return;
}

google_easy_t::google_easy_t(curl_easy_t&& in)
: curl_easy_t{ std::move(in) }
{
    host_set("https://www.googleapis.com");
    return;
}

auto google_easy_t::clear() -> void
{
    curl_easy_t::clear();
    host_set("https://www.googleapis.com");
    return;
}

auto google_easy_t::method() -> void
{
    //Call header_set first then curl_easy_t::method
    //because curl_easy_t::method deals with curl_slist and finalizes the headers.
    header_set("Authorization", "Bearer " + google_account_t::default_get().get_token());
    curl_easy_t::method();
    return;
}
