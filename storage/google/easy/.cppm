export module storage.google.easy;

import std;

import storage.http.curl.easy;

export
{
    class google_easy_t: public curl_easy_t
    {
    public:
        //Returns a google_easy_t whose underlying curl easy handle is distinct from that of curl_easy_t::get().
        static auto get() -> google_easy_t&;

        google_easy_t();
        google_easy_t(curl_easy_t&& in);

        virtual auto clear() -> void override;
    private:
        virtual auto method() -> void override;
    };
}
