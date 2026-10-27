module;

#include <nlohmann/json.hpp>

module storage.google.account;

import std;
import external.curl;
import storage.encode;
import storage.crypto;
import storage.http.curl;

namespace
{
    constexpr auto token_endpoint{ "https://oauth2.googleapis.com/token" };

    // Both signing and network I/O run on the OAuth worker, never under the
    // account mutex or while the GUI is constructing a Drive request.
    class token_request_t : public curl_easy_t
    {
        std::string m_address;
        std::string m_key;
        bool m_signed{};
        unsigned m_retries{};
        std::chrono::steady_clock::time_point m_retry_at{};
        std::chrono::steady_clock::time_point m_attempt_started{};
    public:
        std::string token;
        std::chrono::steady_clock::time_point expires_at{};
        std::chrono::steady_clock::time_point refresh_at{};

        token_request_t(std::string address, std::string key)
            : m_address{ std::move(address) }, m_key{ std::move(key) }
        {
            api_curl(curl_easy_setopt(interface_get(), CURLOPT_VERBOSE, 0L));
        }
        auto prepare() -> bool override
        {
            if (std::chrono::steady_clock::now() < m_retry_at) return false;
            if (m_signed)
            {
                m_attempt_started = std::chrono::steady_clock::now();
                return true;
            }
            const auto now{ std::time(nullptr) };
            const auto header{ base64url_encode(R"({"alg":"RS256","typ":"JWT"})") };
            const auto claims{ base64url_encode(nlohmann::json{
                { "iss", m_address }, { "scope", "https://www.googleapis.com/auth/drive" },
                { "aud", token_endpoint }, { "exp", now + 3600 }, { "iat", now }
            }.dump()) };
            auto assertion{ header + "." + claims };
            try { assertion += "." + base64url_encode(sha256withrsa(assertion, m_key)); }
            catch (...) { throw std::runtime_error{ "Could not sign the OAuth token request." }; }
            m_key.clear();
            host_set(token_endpoint);
            header_set("Content-Type", "application/x-www-form-urlencoded");
            // RFC 7523 uses the full grant-type URN; omitting "grant-type:"
            // makes the OAuth endpoint reject startup authentication with HTTP 400.
            method_post("grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" + assertion);
            m_signed = true;
            m_attempt_started = std::chrono::steady_clock::now();
            return true;
        }
        auto parse() -> void override
        {
            if (!curl_easy_t::valid()) return;
            // Do not expose token endpoint bodies in exceptions or logs.
            try
            {
                const auto response = nlohmann::json::parse(payload());
                token = response.at("access_token").get<std::string>();
                if (!response.at("expires_in").is_number_integer())
                    throw std::invalid_argument{ "Invalid token lifetime type." };
                const auto lifetime{ response.at("expires_in").get<std::int64_t>() };
                if (token.empty() || lifetime <= 0 || lifetime > std::numeric_limits<int>::max())
                    throw std::invalid_argument{ "Invalid token lifetime." };
                // Be conservative about time spent waiting for the response;
                // receiving a delayed reply must not extend the token lifetime.
                expires_at = m_attempt_started + std::chrono::seconds{ lifetime };
                if (expires_at <= std::chrono::steady_clock::now())
                    throw std::invalid_argument{ "Token already expired." };
                refresh_at = expires_at - std::chrono::seconds{ std::min<std::int64_t>(60, lifetime / 10) };
            }
            catch (...) { throw std::runtime_error{ "Invalid OAuth token response." }; }
        }
        auto resume() -> void override
        {
            if (m_response_code >= 400 && m_response_code < 500)
                throw std::runtime_error{ "OAuth token request rejected (HTTP " + std::to_string(m_response_code) + ")." };
            if (++m_retries >= 3)
                throw std::runtime_error{ "OAuth token request failed after three attempts." };
            curl_easy_t::resume();
            m_retry_at = std::chrono::steady_clock::now() + std::chrono::seconds{ m_retries };
        }
    };
}

struct google_account_t::implementation_t
{
    std::mutex mutex;
    std::string address;
    std::string key;
    std::string token;
    std::chrono::steady_clock::time_point expires_at{};
    std::chrono::steady_clock::time_point refresh_at{};
    bool refreshing{};
    bool stopped{};
    std::exception_ptr error;
    // Last: destroy the worker (and its socket callbacks) before account state.
    std::unique_ptr<curl_multi_worker_t> worker;
};

auto google_account_t::default_set(google_account_t account) -> void { default_get() = std::move(account); }
auto google_account_t::default_get() -> google_account_t&
{
    static google_account_t account;
    return account;
}

google_account_t::google_account_t() : m_impl{ std::make_unique<implementation_t>() } {}
google_account_t::google_account_t(std::string address, std::string_view key) : google_account_t{}
{
    init(std::move(address), key);
}
google_account_t::~google_account_t() = default;
google_account_t::google_account_t(google_account_t&&) noexcept = default;
auto google_account_t::operator=(google_account_t&&) noexcept -> google_account_t& = default;

auto google_account_t::init(std::string address, std::string_view key) -> void
{
    auto state{ std::make_unique<implementation_t>() };
    state->address = std::move(address);
    state->key = key;
    m_impl = std::move(state);
}

auto google_account_t::try_token() -> std::optional<std::string>
{
    std::unique_lock lock{ m_impl->mutex };
    if (m_impl->stopped) throw std::runtime_error{ "OAuth token worker stopped." };
    if (m_impl->refreshing)
    {
        try
        {
            m_impl->worker->run();
            if (m_impl->worker->ready_nonempty())
            {
                auto result{ m_impl->worker->ready_get_unique() };
                const auto& response{ dynamic_cast<token_request_t&>(*result) };
                m_impl->token = response.token;
                m_impl->expires_at = response.expires_at;
                m_impl->refresh_at = response.refresh_at;
                m_impl->refreshing = false;
            }
        }
        catch (...)
        {
            m_impl->error = std::current_exception();
            m_impl->refreshing = false;
        }
    }
    const auto now{ std::chrono::steady_clock::now() };
    const bool usable{ !m_impl->token.empty() && now < m_impl->expires_at };
    // A failed proactive refresh does not discard a still-valid token. Once it
    // expires (or is rejected), surface the error instead of endlessly retrying.
    if (!usable && m_impl->error) std::rethrow_exception(m_impl->error);
    if (now >= m_impl->refresh_at && !m_impl->refreshing && !m_impl->error)
    {
        if (m_impl->address.empty() || m_impl->key.empty())
            throw std::runtime_error{ "Google account credentials are not configured." };
        if (!m_impl->worker) m_impl->worker = std::make_unique<curl_multi_worker_t>();
        m_impl->worker->add(token_request_t{ m_impl->address, m_impl->key });
        m_impl->refreshing = true;
    }
    if (usable) return m_impl->token;
    return std::nullopt;
}

auto google_account_t::get_token() -> std::string
{
    while (true)
    {
        if (auto token{ try_token() }) return *token;
        std::this_thread::sleep_for(std::chrono::milliseconds{ 5 });
    }
}

auto google_account_t::reject_token(std::string_view rejected) -> void
{
    std::unique_lock lock{ m_impl->mutex };
    if (m_impl->token == rejected)
    {
        m_impl->token.clear();
        m_impl->expires_at = {};
        m_impl->refresh_at = {};
    }
}

auto google_account_t::stop() -> void
{
    std::unique_ptr<curl_multi_worker_t> worker;
    {
        std::unique_lock lock{ m_impl->mutex };
        m_impl->stopped = true;
        worker = std::move(m_impl->worker);
    }
    // Never hold the account mutex across socket shutdown or thread joins.
    if (worker) worker->stop();
}
