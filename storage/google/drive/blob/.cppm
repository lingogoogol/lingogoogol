module;

#include <nlohmann/json.hpp>

export module storage.google.drive.blob;

import std;

import storage.google.batch;

export
{
	struct blob_t
	{
		std::string m_id{};
		nlohmann::json m_name{};
		nlohmann::json m_content{};
	};

	struct blob_opt_t
	{
		std::string m_id{};
		std::optional<nlohmann::json> m_name{};
		std::optional<nlohmann::json> m_content{};
	};

	struct blob_str_opt_t
	{
		std::string m_id{};
		std::optional<std::string> m_name{};
		std::optional<std::string> m_content{};
	};

	struct blob_data_str_t
	{
		std::string m_name{};
		std::string m_content{};
	};

	struct blob_data_str_opt_t
	{
		std::optional<std::string> m_name{};
		std::optional<std::string> m_content{};
	};

	struct blob_data_boo_t
	{
		bool m_name{};
		bool m_content{};
	};

	auto blob_upload(google_batch_t& batch, std::string name, std::string content) -> void;
	auto blob_update(google_batch_t& batch, std::string id, std::string body) -> void;
	auto blob_update(google_batch_t& batch, blob_str_opt_t blob) -> void;
	auto blob_update(google_batch_t& batch, blob_opt_t blob) -> void;
	auto blob_download(google_batch_t& batch, std::string id) -> void;
}
