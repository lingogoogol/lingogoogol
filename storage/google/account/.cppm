export module storage.google.account;

import std;

export
{
    class google_account_t
    {
        struct implementation_t;
        std::unique_ptr<implementation_t> m_impl;
    public:
        static auto default_set(google_account_t account) -> void;
        static auto default_get() -> google_account_t&;

        google_account_t();
        google_account_t(std::string address, std::string_view key);
        ~google_account_t();
        google_account_t(google_account_t&& in) noexcept;
        auto operator=(google_account_t&& in) noexcept -> google_account_t&;
        // Configure before starting requests; replacing an account stops its old worker.
        auto init(std::string address, std::string_view key) -> void;

        // Poll cached credentials. Starts a shared background refresh when needed;
        // nullopt means callers should keep pumping their event/transfer loop.
        auto try_token() -> std::optional<std::string>;
        // Compatibility helper for synchronous callers; never used to prepare GUI requests.
        auto get_token() -> std::string;
        // A late 401 for an old token must not invalidate a newer cached token.
        auto reject_token(std::string_view rejected) -> void;
        // Wake network waits and make credential waiters fail before joining tasks.
        auto stop() -> void;
    };
}
