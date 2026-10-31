export module storage.google.oauth;
import std;
import storage.state;

export {
    enum class oauth_purpose_t { drive, cloud };
    struct oauth_response_t { long status{}; std::string body; };
    struct oauth_saved_account_t
    {
        std::string address, portrait;
    };
    struct oauth_tokens_t
    {
        std::string access, refresh, nonce;
        std::int64_t expires{};
    };

    // One authorized user and its device-bound DPoP key. Token endpoint work
    // belongs on a background thread; the GUI only polls its account cache.
    class google_oauth_user_t
    {
        struct implementation_t;
        std::unique_ptr<implementation_t> m_impl;
        explicit google_oauth_user_t(std::unique_ptr<implementation_t> impl);
      public:
        ~google_oauth_user_t();
        static auto restore(const std::string& client_path, std::string expected_email = {},
                            oauth_purpose_t purpose = oauth_purpose_t::drive)
            -> std::shared_ptr<google_oauth_user_t>;
        static auto login(const std::string& client_path,
                          const std::shared_ptr<request_control_t>& control,
                          oauth_purpose_t purpose = oauth_purpose_t::drive)
            -> std::shared_ptr<google_oauth_user_t>;
        static auto saved_accounts(const std::string& client_path,
                                   oauth_purpose_t purpose = oauth_purpose_t::drive) -> std::vector<oauth_saved_account_t>;
        static auto select(const std::string& client_path, const std::string& email) -> void;
        static auto sign_out(const std::string& client_path, const std::string& email = {}) -> void;
        static auto remove(const std::string& client_path, const std::string& email,
                           oauth_purpose_t purpose = oauth_purpose_t::drive) -> void;
        // No automatic mutation retries: a lost key-creation response must not
        // silently create more private keys. Only rejected authorization replays.
        auto cloud_request(const std::string& method, const std::string& url, const std::string& body,
                           const std::shared_ptr<request_control_t>& control) -> oauth_response_t;
        auto refresh(const std::shared_ptr<request_control_t>& control) -> oauth_tokens_t;
        auto tokens() const -> oauth_tokens_t;
        auto address() const -> std::string;
        auto credential_id() const -> std::string;
        auto storage_id() const -> std::string;
        auto picture() const -> std::string;
        // Bounded HTTPS download; a missing or unavailable portrait is optional.
        auto download_picture(const std::shared_ptr<request_control_t>& control) const -> std::string;
    };
}
