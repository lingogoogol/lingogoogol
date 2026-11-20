module;

#include <nlohmann/json.hpp>

#include "android/native/bridge.hpp"

module storage.google.oauth;
import std;
import external.curl;
import storage.state;

// Android implementation of the Google user login. The Windows implementation
// (.cpp) runs PKCE with a loopback redirect and keeps refresh tokens and DPoP
// keys in the Windows CNG key storage provider. On Android, Google Play services
// (AuthorizationClient, through MainActivity.kt) issues access tokens for the
// app's Android OAuth client, identified by package name and signing key, so
// client.json is not used and no token is stored: a refresh asks Google Play
// services again for the same account without user interaction.
// Saved users (email, photo) live in oauth/<purpose>/ in the app's data folder.

using json = nlohmann::json;

namespace
{
    constexpr auto drive_scope = "https://www.googleapis.com/auth/drive.appdata";
    constexpr auto cloud_scopes = "https://www.googleapis.com/auth/cloud-platform https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile";
    // Google access tokens last an hour; Google Play services does not report the
    // expiry, so renew a few minutes early.
    constexpr std::int64_t token_lifetime{55 * 60};
    std::mutex vault_mutex;

    auto require(bool condition, const char* message) -> void
    {
        if (!condition) throw std::runtime_error{message};
    }

    auto scope(oauth_purpose_t purpose) -> std::string
    {
        return purpose == oauth_purpose_t::cloud ? cloud_scopes : drive_scope;
    }

    auto directory(oauth_purpose_t purpose) -> std::filesystem::path
    {
        return std::filesystem::path{"oauth"} / (purpose == oauth_purpose_t::cloud ? "cloud" : "drive");
    }

    struct response_t { long status{}; std::string body; };
    auto request(const std::string& url, const std::string& body, const std::vector<std::string>& headers,
                 const std::shared_ptr<request_control_t>& control, const std::string& method = {}) -> response_t
    {
        auto easy = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>{curl_easy_init(), curl_easy_cleanup};
        require(easy != nullptr, "Could not initialize the OAuth connection.");
        curl_slist* list{};
        for (const auto& header : headers)
        {
            auto next = curl_slist_append(list, header.c_str());
            if (!next) { curl_slist_free_all(list); throw std::bad_alloc{}; }
            list = next;
        }
        auto cleanup = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>{list, curl_slist_free_all};
        response_t response;
        curl_easy_setopt(easy.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(easy.get(), CURLOPT_HTTPHEADER, list);
        curl_easy_setopt(easy.get(), CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(easy.get(), CURLOPT_TIMEOUT, 45L);
        curl_easy_setopt(easy.get(), CURLOPT_PROTOCOLS_STR, "https");
        curl_easy_setopt(easy.get(), CURLOPT_VERBOSE, 0L);
        // See storage/http/curl/easy/.cpp: the system roots collected at start-up.
        curl_easy_setopt(easy.get(), CURLOPT_CAINFO, "cacert.pem");
        curl_easy_setopt(easy.get(), CURLOPT_WRITEFUNCTION, +[](char* p, std::size_t a, std::size_t b, void* out) -> std::size_t
        {
            auto& value = static_cast<response_t*>(out)->body;
            if (a * b > 1024 * 1024 - value.size()) return 0;
            value.append(p, a * b);
            return a * b;
        });
        curl_easy_setopt(easy.get(), CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(easy.get(), CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(easy.get(), CURLOPT_XFERINFOFUNCTION, +[](void* p, curl_off_t, curl_off_t, curl_off_t, curl_off_t) -> int
        { return static_cast<request_control_t*>(p)->cancelled ? 1 : 0; });
        curl_easy_setopt(easy.get(), CURLOPT_XFERINFODATA, control.get());
        curl_easy_setopt(easy.get(), CURLOPT_OPENSOCKETFUNCTION, +[](void* p, curlsocktype, struct curl_sockaddr* address) -> curl_socket_t
        {
            return static_cast<curl_socket_t>(static_cast<request_control_t*>(p)->socket_open(
                address->family, address->socktype, address->protocol));
        });
        curl_easy_setopt(easy.get(), CURLOPT_OPENSOCKETDATA, control.get());
        curl_easy_setopt(easy.get(), CURLOPT_CLOSESOCKETFUNCTION, +[](void* p, curl_socket_t socket) -> int
        { return static_cast<request_control_t*>(p)->socket_close(socket); });
        curl_easy_setopt(easy.get(), CURLOPT_CLOSESOCKETDATA, control.get());
        if (!body.empty())
        {
            curl_easy_setopt(easy.get(), CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(easy.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
        }
        if (!method.empty()) curl_easy_setopt(easy.get(), CURLOPT_CUSTOMREQUEST, method.c_str());
        const auto status = curl_easy_perform(easy.get());
        require(!control->cancelled, "Google login was cancelled.");
        require(status == CURLE_OK, "Google OAuth network request failed.");
        curl_easy_getinfo(easy.get(), CURLINFO_RESPONSE_CODE, &response.status);
        return response;
    }
}

struct google_oauth_user_t::implementation_t
{
    oauth_purpose_t purpose{};
    std::string email, photo;
    oauth_tokens_t tokens;
    mutable std::mutex mutex;
    auto save(bool select = false) const -> void
    {
        std::lock_guard lock{vault_mutex};
        const auto folder = directory(purpose);
        const auto id = storage_digest(email);
        storage_atomic_write(folder / (id + ".json"), json{{"email", email}, {"photo", photo}}.dump());
        if (select) storage_atomic_write(folder / "active", id);
    }
};

google_oauth_user_t::google_oauth_user_t(std::unique_ptr<implementation_t> impl) : m_impl{std::move(impl)} {}
google_oauth_user_t::~google_oauth_user_t() = default;

auto google_oauth_user_t::restore(const std::string&, std::string expected_email, oauth_purpose_t purpose)
    -> std::shared_ptr<google_oauth_user_t>
{
    const auto folder = directory(purpose);
    if (expected_email.empty() && !std::filesystem::exists(folder / "active")) return {};
    auto impl = std::make_unique<implementation_t>();
    impl->purpose = purpose;
    try
    {
        std::lock_guard lock{vault_mutex};
        const auto id = expected_email.empty() ? storage_read(folder / "active") : storage_digest(expected_email);
        const auto saved = json::parse(storage_read(folder / (id + ".json")));
        impl->email = saved.at("email");
        impl->photo = saved.value("photo", "");
        require(storage_digest(impl->email) == id && (expected_email.empty() || expected_email == impl->email),
                "Saved Google account does not match.");
    }
    catch (...) { throw std::runtime_error{"Could not restore Google login; sign in again on this phone."}; }
    // Tokens are requested from Google Play services on first use.
    return std::shared_ptr<google_oauth_user_t>{new google_oauth_user_t{std::move(impl)}};
}

auto google_oauth_user_t::login(const std::string&, const std::shared_ptr<request_control_t>& control,
                                oauth_purpose_t purpose) -> std::shared_ptr<google_oauth_user_t>
{
    auto impl = std::make_unique<implementation_t>();
    impl->purpose = purpose;
    // Google Play services shows its account picker and consent screen.
    const auto token = android_authorize({}, scope(purpose), true);
    require(!token.empty(), "Google login was cancelled.");
    require(!control->cancelled, "Google login was cancelled.");
    impl->tokens = {token, {}, {}, std::time(nullptr) + token_lifetime};
    const auto profile_url = purpose == oauth_purpose_t::cloud ? "https://www.googleapis.com/oauth2/v2/userinfo" :
        "https://www.googleapis.com/drive/v3/about?fields=user(emailAddress,photoLink)";
    const auto response = request(profile_url, "", {"Authorization: Bearer " + token}, control);
    require(response.status == 200, "Could not read the signed-in Google profile.");
    try
    {
        const auto data = json::parse(response.body);
        const auto user = purpose == oauth_purpose_t::cloud ? data : data.at("user");
        impl->email = user.at(purpose == oauth_purpose_t::cloud ? "email" : "emailAddress");
        impl->photo = user.value(purpose == oauth_purpose_t::cloud ? "picture" : "photoLink", "");
        if (purpose == oauth_purpose_t::cloud)
            require(user.value("verified_email", false), "Google email address is not verified.");
        require(!impl->email.empty(), "Google profile has no email address.");
    }
    catch (...) { throw std::runtime_error{"Google returned an incomplete user profile."}; }
    impl->save(true);
    return std::shared_ptr<google_oauth_user_t>{new google_oauth_user_t{std::move(impl)}};
}

auto google_oauth_user_t::saved_accounts(const std::string&, oauth_purpose_t purpose) -> std::vector<oauth_saved_account_t>
{
    std::lock_guard lock{vault_mutex};
    const auto folder = directory(purpose);
    std::vector<oauth_saved_account_t> accounts;
    if (!std::filesystem::exists(folder)) return accounts;
    for (const auto& entry : std::filesystem::directory_iterator{folder})
    {
        const auto id = entry.path().stem().string();
        if (entry.path().extension() != ".json" || id.size() != 64) continue;
        try
        {
            const auto email = json::parse(storage_read(entry.path())).at("email").get<std::string>();
            if (storage_digest(email) != id) continue;
            const auto photo = folder / (id + ".photo");
            accounts.push_back({email, std::filesystem::exists(photo) ? storage_read(photo) : std::string{}});
        }
        catch (...) { /* An unreadable entry must not hide the other accounts. */ }
    }
    std::ranges::sort(accounts, {}, &oauth_saved_account_t::address);
    return accounts;
}

auto google_oauth_user_t::select(const std::string&, const std::string& email) -> void
{
    std::lock_guard lock{vault_mutex};
    const auto folder = directory(oauth_purpose_t::drive);
    const auto id = storage_digest(email);
    require(json::parse(storage_read(folder / (id + ".json"))).at("email") == email, "Saved Google account does not match.");
    storage_atomic_write(folder / "active", id);
}

auto google_oauth_user_t::sign_out(const std::string&, const std::string& email) -> void
{
    std::lock_guard lock{vault_mutex};
    const auto active = directory(oauth_purpose_t::drive) / "active";
    // Signing out a background user must preserve another user's selection.
    if (email.empty() || (std::filesystem::exists(active) && storage_read(active) == storage_digest(email)))
        std::filesystem::remove(active);
}

auto google_oauth_user_t::remove(const std::string&, const std::string& email, oauth_purpose_t purpose) -> void
{
    // Like Windows: forget the user on this device only; Google keeps the grant.
    std::lock_guard lock{vault_mutex};
    const auto folder = directory(purpose);
    const auto id = storage_digest(email);
    require(json::parse(storage_read(folder / (id + ".json"))).at("email") == email, "Saved Google account does not match.");
    if (std::filesystem::exists(folder / "active") && storage_read(folder / "active") == id)
        std::filesystem::remove(folder / "active");
    std::filesystem::remove(folder / (id + ".json"));
    std::filesystem::remove(folder / (id + ".photo"));
}

auto google_oauth_user_t::refresh(const std::shared_ptr<request_control_t>& control) -> oauth_tokens_t
{
    std::lock_guard lock{m_impl->mutex};
    require(!control || !control->cancelled, "Google login was cancelled.");
    // Never interactive: like Windows, an expired grant is reported instead of
    // opening a sign-in screen in the middle of an operation.
    const auto token = android_authorize(m_impl->email, scope(m_impl->purpose), false);
    require(!token.empty(), "Google authorization has expired; remove this Google user and add it again.");
    m_impl->tokens = {token, {}, {}, std::time(nullptr) + token_lifetime};
    return m_impl->tokens;
}

auto google_oauth_user_t::tokens() const -> oauth_tokens_t
{
    std::lock_guard lock{m_impl->mutex};
    return m_impl->tokens;
}
auto google_oauth_user_t::address() const -> std::string { return m_impl->email; }
auto google_oauth_user_t::credential_id() const -> std::string { return storage_digest("android:" + m_impl->email); }
auto google_oauth_user_t::storage_id() const -> std::string
{
    return "appdata:" + storage_digest("android") + ":" + m_impl->email;
}
auto google_oauth_user_t::picture() const -> std::string { return m_impl->photo; }

auto google_oauth_user_t::download_picture(const std::shared_ptr<request_control_t>& control) const -> std::string
{
    if (!m_impl->photo.starts_with("https://")) return {};
    try
    {
        // The public photo URL gets no Authorization header or OAuth credentials.
        const auto response = request(m_impl->photo, "", {}, control);
        if (response.status == 200)
        {
            if (response.body.size() <= 256 * 1024)
                try
                {
                    std::lock_guard lock{vault_mutex};
                    storage_atomic_write(directory(m_impl->purpose) / (storage_digest(m_impl->email) + ".photo"),
                                         response.body);
                }
                catch (...) { /* The portrait remains usable when caching fails. */ }
            return response.body;
        }
    }
    catch (...) {}
    return {};
}

auto google_oauth_user_t::cloud_request(const std::string& method, const std::string& url, const std::string& body,
                                        const std::shared_ptr<request_control_t>& control) -> oauth_response_t
{
    require(m_impl->purpose == oauth_purpose_t::cloud, "Cloud authorization is required.");
    require(url.starts_with("https://cloudresourcemanager.googleapis.com/") ||
                url.starts_with("https://serviceusage.googleapis.com/") ||
                url.starts_with("https://iam.googleapis.com/"),
            "Unexpected Google Cloud endpoint.");
    for (int attempt{}; attempt < 2; ++attempt)
    {
        auto token = tokens();
        if (token.expires <= std::time(nullptr) + 60) token = refresh(control);
        const auto response = request(url, body, {"Authorization: Bearer " + token.access,
            "Content-Type: application/json; charset=UTF-8"}, control, method);
        if (response.status != 401 || attempt) return {response.status, response.body};
        refresh(control);
    }
    throw std::runtime_error{"Google Cloud authorization failed."};
}
