module storage.google.easy;

import std;
import storage.google.account;

auto google_easy_t::get() -> google_easy_t&
{
    thread_local google_easy_t easy{};
    return easy;
}

google_easy_t::google_easy_t() : curl_easy_t{} { host_set("https://www.googleapis.com"); }
google_easy_t::google_easy_t(curl_easy_t&& in) : curl_easy_t{ std::move(in) }
{
    host_set("https://www.googleapis.com");
}

auto google_easy_t::clear() -> void
{
    curl_easy_t::clear();
    host_set("https://www.googleapis.com");
    m_sent_token.clear();
    m_auth_retries = 0;
}

auto google_easy_t::method() -> void
{
    // Request construction is GUI-safe: OAuth I/O/signing is deferred until
    // prepare(), immediately before the attempt is attached to CURLM.
    m_auth_retries = 0;
    curl_easy_t::method();
}

auto google_easy_t::prepare() -> bool
{
    auto token{ google_account_t::default_get().try_token() };
    if (!token) return false;
    m_sent_token = std::move(*token);
    header_set("Authorization", "Bearer " + m_sent_token);
    // Rebuild curl_slist as well as the header map, preserving method and body.
    curl_easy_t::method();
    return true;
}

auto google_easy_t::authorization_rejected() const -> bool { return m_response_code == 401; }

auto google_easy_t::resume() -> void
{
    if (authorization_rejected())
    {
        if (m_auth_retries++ != 0)
            throw std::runtime_error{ "Google API rejected refreshed credentials (HTTP 401)." };
        google_account_t::default_get().reject_token(m_sent_token);
    }
    curl_easy_t::resume();
}
