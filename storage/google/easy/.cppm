export module storage.google.easy;

import std;
import storage.http.curl.easy;

export
{
    class google_easy_t: public curl_easy_t
    {
        std::string m_sent_token;
        unsigned m_auth_retries{};
    public:
        // Distinct from curl_easy_t::get(), and local to the executing thread.
        static auto get() -> google_easy_t&;
        google_easy_t();
        google_easy_t(curl_easy_t&& in);
        auto clear() -> void override;
        auto prepare() -> bool override;
        auto resume() -> void override;
    protected:
        // Batch requests can carry a 401 inside an otherwise successful response.
        virtual auto authorization_rejected() const -> bool;
    private:
        auto method() -> void override;
    };
}
