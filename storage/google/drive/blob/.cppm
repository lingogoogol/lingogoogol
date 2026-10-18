module;

#include <nlohmann/json.hpp>

export module storage.google.drive.blob;

import std;

import storage.google.easy;
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

	struct blob_boo_t
	{
		std::string m_id{};
		bool m_name{};
		bool m_content{};
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

	auto blob_upload(google_easy_t& easy, std::string name, std::string content) -> void;
	auto blob_upload(google_batch_t& batch, std::string name, std::string content) -> void;
	auto blob_upload_run(google_easy_t& easy, std::string name, std::string content) -> std::string;

	auto blob_update(google_easy_t& easy, blob_opt_t blob) -> void;
	auto blob_update(google_easy_t& easy, blob_str_opt_t blob) -> void;
	auto blob_update(google_batch_t& batch, std::string id, std::string payload) -> void;
	auto blob_update(google_batch_t& batch, blob_opt_t blob) -> void;
	auto blob_update(google_batch_t& batch, blob_str_opt_t blob) -> void;
	auto blob_update_run(google_easy_t& easy, blob_str_opt_t blob) -> void;
	auto blob_update_name(google_easy_t& easy, std::string id, std::string name) -> void;
	auto blob_update_name_run(google_easy_t& easy, std::string id, std::string name) -> void;
	auto blob_update_content(google_easy_t& easy, std::string id, std::string content) -> void;
	auto blob_update_content_run(google_easy_t& easy, std::string id, std::string content) -> void;

	auto blob_exist_run(std::string id) -> bool;
	
	auto blob_get(google_easy_t& easy, std::string id, std::string field) -> void;
	auto blob_get(google_batch_t& batch, std::string id, std::string field) -> void;
	auto blob_get_run(google_easy_t& easy, std::string id, std::string field) -> nlohmann::json;
	auto blob_get_run(google_easy_t& easy, blob_boo_t blob) -> blob_data_str_t;
	auto blob_get_name(google_easy_t& easy, std::string id) -> void;
	auto blob_get_name_run(google_easy_t& easy, std::string id) -> std::string;
	auto blob_get_content(google_easy_t& easy, std::string id) -> void;
	auto blob_get_content_run(google_easy_t& easy, std::string id) -> std::string;
}
