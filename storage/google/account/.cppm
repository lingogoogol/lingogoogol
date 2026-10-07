export module storage.google.account;

import std;

import external.curl;

import storage.http.curl;

export
{
	class google_account_t
	{
	public:
        static auto default_set(google_account_t account) -> void;
        static auto default_get() -> google_account_t&;

		google_account_t() = default;
		google_account_t(std::string address, std::string_view key);
		auto init(std::string address, std::string_view key) -> void;
		google_account_t(google_account_t&& in);
		auto operator=(google_account_t&& in) -> google_account_t&;

		auto get_token() -> std::string;
	private:
		std::string m_address{};
		std::string m_key{};
		std::string m_token{};
		std::time_t m_token_life{};
		curl_easy_t m_easy{};
		std::mutex m_mutex{};

		//Set host and path for token retrieval.
		auto init1() -> void;
	};
}
