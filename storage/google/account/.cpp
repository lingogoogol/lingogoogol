module;

#include <nlohmann/json.hpp>

module storage.google.account;

import std;

import external.curl;

import storage.encode;
import storage.crypto;
import storage.http.curl;

auto google_account_t::default_set(google_account_t account) -> void
{
    default_get() = std::move(account);
    return;
}

auto google_account_t::default_get() -> google_account_t&
{
    static google_account_t account{};
    return account;
}

google_account_t::google_account_t(std::string address, std::string_view key)
: m_address{ address }, m_key{ key }
{
    init1();
    return;
}

auto google_account_t::init(std::string address, std::string_view key) -> void
{
    m_address = address;
    m_key = key;
    init1();
    return;
}

google_account_t::google_account_t(google_account_t&& in)
{
    *this = std::move(in);
    return;
}

auto google_account_t::operator=(google_account_t&& in) -> google_account_t&
{
    std::unique_lock lock{ m_mutex };
    std::unique_lock lock_in{ in.m_mutex };
    m_address = std::move(in.m_address);
    m_key = std::move(in.m_key);
    m_token = std::move(in.m_token);
    m_token_life = in.m_token_life;
    m_easy = std::move(in.m_easy);
    return *this;
}

auto google_account_t::get_token() -> std::string
{
    std::unique_lock lock{ m_mutex };
    std::time_t time{ std::time(nullptr) };
    if (time >= m_token_life)
    {
        std::string encoded
        {
            base64url_encode
            (
                "{"
                    "\"alg\":\"RS256\","
                    "\"typ\":\"JWT\""
                "}"
            ) + "." +
            base64url_encode
            (
                "{"
                    "\"iss\":\"" + m_address + "\","
                    "\"scope\":\"https://www.googleapis.com/auth/drive\","
                    "\"aud\":\"https://oauth2.googleapis.com/token\","
                    "\"exp\":" + std::to_string(time + 3600) + ","
                    "\"iat\":" + std::to_string(time) +
                "}"
            )
        };
        std::string body
        {
            "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" +
            encoded + "." + base64url_encode(sha256withrsa(encoded, m_key))
        };
        std::string response{ m_easy.method_post_run(body) };
        m_token = nlohmann::json::parse(response)["access_token"].get<std::string>();
        m_token_life = time + 3600 / 2;
    }
    return m_token;
}

auto google_account_t::init1() -> void
{
    m_easy.host_set("https://oauth2.googleapis.com");
    m_easy.path_set("/token");
    return;
}
