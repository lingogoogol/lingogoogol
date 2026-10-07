export module storage.google.drive.blob_queue;

import std;

import storage.google.account;

export
{
	class blob_queue_t
	{
	private:
		std::deque<std::string> m_id{};
		std::mutex m_mutex{};
		std::thread m_thread{};
		std::condition_variable m_condition_variable{};
		std::filesystem::path m_path{};
		google_account_t* m_account{};
		const std::atomic_bool* m_close{};
		bool m_clearing{ false };
		bool m_cleared{ false };

		auto process() -> void;
	public:
		blob_queue_t(std::filesystem::path path, google_account_t* account, const std::atomic_bool* close);
		~blob_queue_t();

		auto get() -> std::string;
		auto get(std::uint64_t count) -> std::vector<std::string>;

		auto clear_begin() -> void;
		auto clear_end() -> void;
	};
}
