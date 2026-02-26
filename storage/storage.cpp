#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
#include <fstream>
#include <map>

#define UNICODE

#include "multipart.h"
#include "api.h"

#include "../compilercpp/lib/.hpp"
#include "../compilercpp/GUI/.hpp"

auto curl_main() -> CURL*& {
	static CURL* out{};
	return out;
}

auto curl_upload() -> CURL*& {
	static CURL* out{};
	return out;
}

auto curl_download() -> CURL*& {
	static CURL* out{};
	return out;
}

auto init() -> void {
	api_curl(curl_global_init(CURL_GLOBAL_DEFAULT));
	curl_main() = curl_easy_init();
	return;
}

auto free() -> void {
	curl_easy_cleanup(curl_main());
	curl_global_cleanup();
	return;
}

account_t account_main{};

auto http_api_try(CURL* curl, std::string url, std::vector<std::pair<std::string, std::string>> query
, std::vector<std::string> header, account_t& account) -> std::string {
	if (!query.empty()) {
		auto i{ query.begin() };
		url += "?" + i->first + "=" + i->second;
		++i;
		for (; i != query.end(); ++i) {
			url += "&" + i->first + "=" + i->second;
		}
	}
	curl_slist* header_curl{};
	header_curl = curl_slist_append(header_curl, ("Authorization: Bearer " + account.get_token()).c_str());
	for (auto i : header) {
		header_curl = curl_slist_append(header_curl, i.c_str());
	}
	std::string response{};
	debug_str_t debug_str{};
    while (true) {
        try {
			api_curl(curl_easy_setopt(curl, CURLOPT_URL, url.data()));
			api_curl(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_curl));
			api_curl(curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(&response)));
			api_curl(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback));
			api_curl(curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L));
			api_curl(curl_easy_setopt(curl, CURLOPT_DEBUGDATA, static_cast<void*>(&debug_str)));
			api_curl(curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, &debug_callback));
			api_curl(curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L));
			api_curl(curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L));
			api_curl(curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 30L));
			api_curl(curl_easy_perform(curl));
			api_curl(curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L));
        }
        catch (error_curl_t error) {
            log_file(error.what() + "\n");
            continue;
        }
        break;
    }
	curl_slist_free_all(header_curl);
	return response;
}

auto http_api(CURL* curl, std::string url, std::vector<std::pair<std::string, std::string>> query
, std::vector<std::string> header, account_t& account) -> std::string {
	std::string result{};
	long response_code{};
	do {
		result = http_api_try(curl, url, query, header, account);
		api_curl(curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code));
	} while (response_code / 100 != 2);
	return result;
}

class dialog_event_t: public IFileDialogEvents {
private:
	bool m_ok{ false };
	std::vector<std::string> m_result{};
public:
	auto ok() -> bool {
		return m_ok;
	}

	auto result() -> std::vector<std::string> {
		return m_result;
	}

	auto OnFileOk(IFileDialog* in) -> HRESULT override {
		IFileOpenDialog* dialog{};
		in->QueryInterface(IID_PPV_ARGS(&dialog));
		IShellItemArray* result_array{};
		dialog->GetResults(&result_array);
		DWORD result_count{};
		result_array->GetCount(&result_count);
		m_result.resize(result_count);
		for (DWORD i{ 0 }; i < result_count; ++i) {
			IShellItem* result{};
			result_array->GetItemAt(i, &result);
			LPWSTR result_strc{};
			result->GetDisplayName(SIGDN_FILESYSPATH, &result_strc);
			m_result[i] = to_string(result_strc);
		}
		m_ok = true;
		return S_OK;
	}
	
	auto OnFolderChange(IFileDialog*) -> HRESULT override { return S_OK; }
	auto OnFolderChanging(IFileDialog*, IShellItem*) -> HRESULT override { return S_OK; }
	auto OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) -> HRESULT override { return S_OK; }
	auto OnSelectionChange(IFileDialog*) -> HRESULT override { return S_OK; }
	auto OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) -> HRESULT override { return S_OK; }
	auto OnTypeChange(IFileDialog*) -> HRESULT override { return S_OK; }
	auto AddRef() -> ULONG override { return 0; }
	auto Release() -> ULONG override { return 0; }
	auto QueryInterface(REFIID, void**) -> HRESULT override { throw; }
};

struct loading_t {
	std::string m_type{};
	std::string m_name{};
	std::uint64_t m_size{};
	std::atomic_bool m_processed{ false };
	std::string m_pos{};
	std::atomic_uint64_t m_progress{ 0 };
	std::uint64_t m_progress_bar_id{};
};

struct loading_folder_t {
	std::string m_name{};
	std::uint64_t m_size{};
	std::uint64_t m_progress{ 0 };
	std::shared_ptr<text_line_scroll_std_t> m_progress_bar{};
};

struct loading_file_t {
	std::string m_name{};
	std::uint64_t m_size{};
	std::uint64_t m_progress{ 0 };
	std::string m_parent{};
	std::string m_main_id{};
	std::string m_main_last_id{ "null" };
	std::string m_main_last_last{};
	std::string m_last_id{ "null" };
	std::string m_last_last{};
	std::uint64_t m_last_size{};
	std::shared_ptr<text_line_scroll_std_t> m_progress_bar{};
	std::shared_ptr<loading_folder_t> m_folder{};
	loading_t* m_loading{};
};

struct delete_file_t {
	std::string m_id{};
	std::shared_ptr<text_line_scroll_std_t> m_progress_bar{};
	loading_t* m_loading{};
};

std::atomic_bool close_app{ false };

struct loading_list_t {
	std::deque<std::shared_ptr<loading_t>> m_list{};
	std::deque<std::shared_ptr<loading_t>> m_list_process{};
	std::shared_ptr<divy_soft_t> m_progress_bar_div{};
	std::mutex m_mutex{};
};

auto message_loop(engine_t* engine, loading_list_t* uploading_list, loading_list_t* downloading_list) -> bool {
	std::unique_lock<std::mutex> upload_lock{ uploading_list->m_mutex };
	if (!uploading_list->m_list.empty()) {
		auto& uploading{ uploading_list->m_list.front() };
		if (uploading->m_processed && uploading->m_progress == uploading->m_size) {
			uploading_list->m_progress_bar_div->remove_object(uploading->m_progress_bar_id);
			uploading_list->m_list.pop_front();
		}
	}
	upload_lock.unlock();
	std::unique_lock<std::mutex> download_lock{ downloading_list->m_mutex };
	if (!downloading_list->m_list.empty()) {
		auto& downloading{ downloading_list->m_list.front() };
		if (downloading->m_processed && downloading->m_progress == downloading->m_size) {
			downloading_list->m_progress_bar_div->remove_object(downloading->m_progress_bar_id);
			downloading_list->m_list.pop_front();
		}
	}
	download_lock.unlock();
	return engine->message_loop();
}

auto dialog_file(engine_t* engine, bool folder, loading_list_t* uploading_list, loading_list_t* downloading_list) -> std::vector<std::string> {
	IFileDialog* dialog{};
	CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
	dialog_event_t dialog_event{};
	DWORD dialog_event_id{};
	dialog->Advise(&dialog_event, &dialog_event_id);
	FILEOPENDIALOGOPTIONS dialog_option{};
	dialog->GetOptions(&dialog_option);
	dialog->SetOptions(dialog_option | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT | (folder ? FOS_PICKFOLDERS : 0));
	dialog->Show(nullptr);
	while (!dialog_event.ok()) {
		message_loop(engine, uploading_list, downloading_list);
	}
	return dialog_event.result();
}

auto to_date_time(FILETIME file) -> std::string {
	SYSTEMTIME system{};
	FileTimeToSystemTime(&file, &system);
	return std::to_string(system.wYear) + "-" + std::to_string(system.wMonth) + "-" + std::to_string(system.wDay)
	+ "T" + std::to_string(system.wHour) + ":" + std::to_string(system.wMinute) + ":" + std::to_string(system.wSecond) + "Z";
}

auto file_exist(std::string id) -> bool {
	api_curl(curl_easy_setopt(curl_main(), CURLOPT_HTTPGET, 1L));
	std::string response{ http_api(curl_main(), "https://www.googleapis.com/drive/v3/files/" + id, {}, {}, account_main) };
	return !nlohmann::json::parse(response).contains("error");
}

auto data_upload(CURL* curl, std::string name, std::string content) -> std::string {
	std::string metadata{ nlohmann::json{
		{ "name", name },
		{ "description", content }
	}.dump() };
	api_curl(curl_easy_setopt(curl, CURLOPT_POST, 1L));
	api_curl(curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(metadata.size())));
	api_curl(curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, metadata.data()));
	std::string response{ http_api(curl, "https://www.googleapis.com/drive/v3/files", {
		{ "fields", "id" }
	}, {
		{ "Content-Type: application/json; charset=UTF-8" }
	}, account_main) };
	return nlohmann::json::parse(response)["id"];
}

auto data_get(CURL* curl, std::string id) -> data_t {
	api_curl(curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L));
	std::string response_str{ http_api(curl, "https://www.googleapis.com/drive/v3/files/" + id, {
		{ "fields", "name,description" }
	}, {}, account_main) };
	nlohmann::json response{ nlohmann::json::parse(response_str) };
	return data_t{ response["name"], response["description"] };
}

auto data_name_get(CURL* curl, std::string id) -> std::string {
	api_curl(curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L));
	std::string response{ http_api(curl, "https://www.googleapis.com/drive/v3/files/" + id, {
		{ "fields", "name" }
	}, {}, account_main) };
	return nlohmann::json::parse(response)["name"];
}

auto data_content_get(CURL* curl, std::string id) -> std::string {
	api_curl(curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L));
	std::string response{ http_api(curl, "https://www.googleapis.com/drive/v3/files/" + id, {
		{ "fields", "description" }
	}, {}, account_main) };
	return nlohmann::json::parse(response)["description"];
}

auto data_update(CURL* curl, std::string id, std::string request) -> void {
	api_curl(curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH"));
	read_src_t data{ request };
	api_curl(curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L));
	api_curl(curl_easy_setopt(curl, CURLOPT_READDATA, static_cast<void*>(&data)));
	api_curl(curl_easy_setopt(curl, CURLOPT_READFUNCTION, &read_callback));
	http_api(curl, "https://www.googleapis.com/drive/v3/files/" + id, {
		{ "fields", "description" }
	}, {}, account_main);
	api_curl(curl_easy_setopt(curl, CURLOPT_UPLOAD, 0L));
	api_curl(curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, nullptr));
	api_curl(curl_easy_setopt(curl, CURLOPT_READFUNCTION, nullptr));
	return;
}

auto data_set(CURL* curl, std::string id, data_t data) -> void {
	data_update(curl, id, nlohmann::json{
		{ "name", data.m_name },
		{ "description", data.m_content }
	}.dump());
	return;
}

auto data_name_set(CURL* curl, std::string id, std::string name) -> void {
	data_update(curl, id, nlohmann::json{
		{ "name", name }
	}.dump());
	return;
}

auto data_content_set(CURL* curl, std::string id, std::string content) -> void {
	data_update(curl, id, nlohmann::json{
		{ "description", content }
	}.dump());
	return;
}

auto data_upload(curl_multipart_t& multipart, blob_empty_queue_t& empty_blob, std::string name, std::string content) -> std::string {
	std::string id{ empty_blob.get() };
	data_t data{};
	data.m_name = name;
	data.m_content = content;
	data_set_request(multipart, id, data);
	return id;
}

auto folder_data_add(CURL* curl, std::string main_id, std::string last_id) -> std::string {
	std::string id{ data_upload(curl, nlohmann::json{
		{ "main", main_id },
		{ "last", last_id },
		{ "next", "null" }
	}.dump(), nlohmann::json::array().dump()) };
	if (last_id != "null") {
		nlohmann::json last_json{ nlohmann::json::parse(data_name_get(curl, last_id)) };
		last_json["next"] = id;
		data_name_set(curl, last_id, last_json.dump());
	}
	return id;
}

constexpr std::uint64_t content_size_max{ 25000 };
constexpr std::uint64_t content_size_max_base64{ 18750 };

auto folder_add(CURL* curl, std::string pos, nlohmann::json obj) -> void {
	nlohmann::json folder{ nlohmann::json::parse(data_name_get(curl, pos)) };
	if (folder["end"] == "null") {
		std::string data_id{ folder_data_add(curl, pos, "null") };
		folder["begin"] = data_id;
		folder["end"] = data_id;
	}
	nlohmann::json last_name{ nlohmann::json::parse(data_name_get(curl, folder["end"])) };
	nlohmann::json last_content{ nlohmann::json::parse(data_content_get(curl, folder["end"])) };
	last_content.push_back(obj);
	std::string content{ last_content.dump() };
	if (content.size() > content_size_max) {
		std::string data_id{ folder_data_add(curl, pos, folder["end"]) };
		last_name["next"] = data_id;
		data_name_set(curl, folder["end"], last_name.dump());
		folder["end"] = data_id;
		data_name_set(curl, pos, folder.dump());
		nlohmann::json array{ nlohmann::json::array() };
		array.push_back(obj);
		data_content_set(curl, data_id, array.dump());
	}
	else {
		data_content_set(curl, folder["end"], content);
	}
	folder["count"] = folder["count"] + 1;
	data_name_set(curl, pos, folder.dump());
	return;
}

std::string folder_root_name{ "folder_root" };
std::string folder_root_id{ "1u8tdFhR_NSETA8CNbCg_mxszd8GhPa7e" };

auto folder_root_create(CURL* curl) -> std::string {
	return data_upload(curl, nlohmann::json{
		{ "name", folder_root_name },
		{ "parent", "null" },
		{ "begin", "null" },
		{ "end", "null" },
		{ "count", 0 }
	}.dump(), nlohmann::json::array().dump());
}

auto folder_create_callback(engine_t* engine, state_t* state, std::string* pos) -> void {
	return;
}

auto folder_data_add(blob_empty_queue_t& empty_blob, std::map<std::string, blob_t>& data
, std::string main_id, std::string last_id) -> std::string {
	blob_t current{};
	current.m_id = empty_blob.get();
	current.m_name = nlohmann::json{
		{ "main", main_id },
		{ "last", last_id },
		{ "next", "null" },
		{ "size", 0 }
	};
	current.m_content = nlohmann::json::array();
	if (last_id != "null") {
		nlohmann::json last_name{ data[last_id].m_name };
		last_name["next"] = current.m_id;
		data[last_id].m_name = last_name;
	}
	data[current.m_id] = current;
	return current.m_id;
}

auto folder_add(blob_empty_queue_t& empty_blob, blob_t& main, std::map<std::string, blob_t>& data
, std::string pos, nlohmann::json obj) -> void {
	if (main.m_name["end"] == "null") {
		std::string data_id{ folder_data_add(empty_blob, data, pos, "null") };
		main.m_name["begin"] = data_id;
		main.m_name["end"] = data_id;
	}
	nlohmann::json last_content{ data[main.m_name["end"]].m_content };
	last_content.push_back(obj);
	std::string content{ last_content.dump() };
	if (content.size() > content_size_max) {
		std::string data_id{ folder_data_add(empty_blob, data, pos, main.m_name["end"]) };
		data[main.m_name["end"]].m_name["next"] = data_id;
		main.m_name["end"] = data_id;
		nlohmann::json array{ nlohmann::json::array() };
		array.push_back(obj);
		data[data_id].m_content = array;
	}
	else {
		data[main.m_name["end"]].m_content = last_content;
	}
	main.m_name["count"] = main.m_name["count"] + 1;
	return;
}

auto file_upload(curl_multipart_t& multipart, blob_empty_queue_t& empty_blob, loading_t* loading
, std::shared_ptr<loading_folder_t> loading_folder, std::shared_ptr<divy_soft_t> progress_bar_div
, std::string pos, std::filesystem::path file, std::deque<loading_file_t>* list, std::mutex& mutex) -> std::string {
	loading_file_t uploading{};
	uploading.m_name = file.string();
	if (progress_bar_div) {
		std::unique_lock<std::mutex> lock{ mutex };
		uploading.m_progress_bar = progress_bar_div->add_object_shared<text_line_scroll_std_t>(depth_range_t{0.0f,0.0f}, "檔案：0B/?B " + file.string(), std_blue_light, size_1D{ 0x100 });
		lock.unlock();
	}
	uploading.m_size = std::filesystem::file_size(file);
	if (loading_folder) {
		loading_folder->m_size += uploading.m_size;
		uploading.m_folder = loading_folder;
	}
	loading->m_size += uploading.m_size;
	uploading.m_loading = loading;
	uploading.m_parent = pos;
	uploading.m_main_id = empty_blob.get();
	data_t data{};
	data.m_name = nlohmann::json{
		{ "name", file.filename() },
		{ "parent", pos },
		{ "size", uploading.m_size },
		{ "last", "null" },
		{ "next", "null" }
	}.dump();
	data.m_content = nlohmann::json::array().dump();
	data_set_request(multipart, uploading.m_main_id, data);
	http_api_request_action(multipart);
	if (progress_bar_div) {
		uploading.m_progress_bar->set_text("檔案：0B/" + std::to_string(uploading.m_size) + "B " + file.string());
	}
	list->emplace_back(uploading);
	log_file(file.string() + "\n");
	return uploading.m_main_id;
}

auto folder_upload(curl_multipart_t& multipart, blob_empty_queue_t& empty_blob, loading_t* loading
, std::shared_ptr<loading_folder_t> loading_folder, std::shared_ptr<divy_soft_t> progress_bar_div
, std::string pos, std::filesystem::path folder, std::deque<loading_file_t>* uploading_list, std::mutex& mutex) -> std::string {
	blob_t main{};
	main.m_id = empty_blob.get();
	main.m_name = nlohmann::json{
		{ "name", folder.filename().string() },
		{ "parent", pos },
		{ "begin", "null" },
		{ "end", "null" },
		{ "count", 0 }
	};
	main.m_content = nlohmann::json::array();
	std::map<std::string, blob_t> data{};
	for (std::filesystem::directory_iterator j{ folder }; j != std::filesystem::directory_iterator{}; ++j) {
		if (j->is_regular_file()) {
			std::string id{ file_upload(multipart, empty_blob, loading, loading_folder
			, progress_bar_div, main.m_id, j->path(), uploading_list, mutex) };
			folder_add(empty_blob, main, data, main.m_id, nlohmann::json{
				{ "name", j->path().filename().string() },
				{ "type", "file" },
				{ "size", std::filesystem::file_size(j->path()) },
				{ "id", id }
			});
		}
		else if (j->is_directory()) {
			std::string id{ folder_upload(multipart, empty_blob, loading, loading_folder
			, progress_bar_div, main.m_id, j->path(), uploading_list, mutex) };
			folder_add(empty_blob, main, data, main.m_id, nlohmann::json{
				{ "name", j->path().filename().string() },
				{ "type", "folder" },
				{ "id", id }
			});
		}
	}
	for (auto i : data) {
		data_set_request(multipart, i.second);
		http_api_request_action(multipart);
	}
	data_set_request(multipart, main);
	http_api_request_action(multipart);
	log_file(folder.string() + "\n");
	return main.m_id;
}

auto folder_upload_callback(engine_t* engine, std::shared_ptr<divy_soft_t> progress_bar_div, std::string* pos, loading_list_t* uploading_list, loading_list_t* downloading_list) -> void {
	std::vector<std::string> folder{ dialog_file(engine, true, uploading_list, downloading_list) };
	for (auto i : folder) {
		auto loading{ std::make_shared<loading_t>() };
		loading->m_type = "folder";
		loading->m_name = i;
		loading->m_pos = *pos;
		loading->m_progress_bar_id = progress_bar_div->add_object<divy_flex_soft_t>(depth_range_t{0.0f,0.5f}, progress_bar_div->space_x(std_margin)
    	, std_margin, alignment_2D{ alignment_x::left, alignment_y::bottom }, true);
		std::unique_lock lock{ uploading_list->m_mutex };
		uploading_list->m_list.push_back(loading);
		uploading_list->m_list_process.push_back(loading);
		lock.unlock();
	}
	return;
}

auto file_upload_callback(engine_t* engine, std::shared_ptr<divy_soft_t> progress_bar_div, std::string* pos, loading_list_t* uploading_list, loading_list_t* downloading_list) -> void {
	std::vector<std::string> file{ dialog_file(engine, false, uploading_list, downloading_list) };
	for (auto i : file) {
		auto loading{ std::make_shared<loading_t>() };
		loading->m_type = "file";
		loading->m_name = i;
		loading->m_pos = *pos;
		loading->m_progress_bar_id = progress_bar_div->add_object<divy_flex_soft_t>(depth_range_t{0.0f,0.5f}, progress_bar_div->space_x(std_margin)
    	, std_margin, alignment_2D{ alignment_x::left, alignment_y::bottom }, true);
		std::unique_lock lock{ uploading_list->m_mutex };
		uploading_list->m_list.push_back(loading);
		uploading_list->m_list_process.push_back(loading);
		lock.unlock();
	}
	return;
}

auto list(engine_t* engine, std::shared_ptr<divy_soft_t> download_progress_bar_div, std::shared_ptr<divy_soft_t> viewer_div, std::string* pos, loading_list_t* uploading_list, loading_list_t* downloading_list) -> void;

auto delete_all_confirm_callback(std::shared_ptr<divy_hard_t> center_div, std::shared_ptr<divy_soft_t> progress_bar_div, std::shared_ptr<text_line_flex_std_input_t> input, loading_list_t* uploading_list) {
	if (input->get_text() == "清除") {
		auto loading{ std::make_shared<loading_t>() };
		loading->m_type = "delete_all";
		loading->m_name = "清除";
		loading->m_progress_bar_id = progress_bar_div->add_object<divy_flex_soft_t>(depth_range_t{0.0f,0.5f}, progress_bar_div->space_x(std_margin)
    	, std_margin, alignment_2D{ alignment_x::left, alignment_y::bottom }, true);
		std::unique_lock lock{ uploading_list->m_mutex };
		uploading_list->m_list.push_back(loading);
		uploading_list->m_list_process.push_back(loading);
		lock.unlock();
	}
	center_div->clear_state();
	return;
}

auto delete_all_cancel_callback(std::shared_ptr<divy_hard_t> center_div) {
	center_div->clear_state();
	return;
}

auto delete_all_callback(std::shared_ptr<divy_hard_t> center_div, std::shared_ptr<divy_soft_t> progress_bar_div, loading_list_t* uploading_list) -> void {
	auto input_div{ center_div->add_object_shared<divy_flex_soft_t>(depth_range_t{0.0f,0.5f}, size_1D{ 0x400 }, std_margin, alignment_2D{ alignment_x::left, alignment_y::center }) };
	input_div->add_object<text_line_flex_std_t>(depth_range_t{0.0f,0.0f}, "真的要清除的話輸入\"清除\"", std_sideways, std_red);
	auto focus{ new focus_t{} };
	auto input{ input_div->add_object_shared<text_line_flex_std_input_t>(depth_range_t{0.0f,0.0f}, "", focus, std_sideways, std_red) };
	input_div->add_object<button_classic_std_t>(depth_range_t{0.0f,0.5f}, std::bind(&delete_all_confirm_callback, center_div, progress_bar_div, input, uploading_list), "確認");
	input_div->add_object<button_classic_std_t>(depth_range_t{0.0f,0.5f}, std::bind(&delete_all_cancel_callback, center_div), "取消");
	return;
}

auto folder_button_callback(engine_t* engine, std::shared_ptr<divy_soft_t> download_progress_bar_div, std::shared_ptr<divy_soft_t> viewer_div, std::string* pos_ptr, std::string pos, loading_list_t* uploading_list, loading_list_t* downloading_list) -> void {
	*pos_ptr = pos;
	list(engine, download_progress_bar_div, viewer_div, pos_ptr, uploading_list, downloading_list);
	return;
}

auto folder_download_callback(engine_t* engine, std::shared_ptr<divy_soft_t> progress_bar_div, std::string name, std::string pos, loading_list_t* uploading_list, loading_list_t* downloading_list) -> void {
	std::vector<std::string> folder{ dialog_file(engine, true, uploading_list, downloading_list) };
	if (folder.size() != 1) {
		throw;
	}
	auto loading{ std::make_shared<loading_t>() };
	loading->m_type = "folder";
	loading->m_name = (std::filesystem::path{ folder[0] } / name).string();
	loading->m_pos = pos;
	loading->m_progress_bar_id = progress_bar_div->add_object<divy_flex_soft_t>(depth_range_t{0.0f,0.5f}, progress_bar_div->space_x(std_margin)
	, std_margin, alignment_2D{ alignment_x::left, alignment_y::bottom }, true);
	std::unique_lock lock{ downloading_list->m_mutex };
	downloading_list->m_list.push_back(loading);
	downloading_list->m_list_process.push_back(loading);
	lock.unlock();
	return;
}

auto list_folder_content(CURL* curl, std::string* pos) -> nlohmann::json {
	api_curl(curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L));
	data_t response_data{ data_get(curl, *pos) };
	std::string next{ nlohmann::json::parse(response_data.m_name)["begin"] };
	nlohmann::json obj_list{ nlohmann::json::array() };
	while (next != "null") {
		response_data = data_get(curl, next);
		nlohmann::json response_content{ nlohmann::json::parse(response_data.m_content) };
		obj_list.insert(obj_list.end(), response_content.begin(), response_content.end());
		next = nlohmann::json::parse(response_data.m_name)["next"];
	}
	return obj_list;
}

auto file_download_callback(engine_t* engine, std::shared_ptr<divy_soft_t> progress_bar_div, std::string name, std::string pos, loading_list_t* uploading_list, loading_list_t* downloading_list) -> void {
	std::vector<std::string> folder{ dialog_file(engine, true, uploading_list, downloading_list) };
	if (folder.size() != 1) {
		throw;
	}
	auto loading{ std::make_shared<loading_t>() };
	loading->m_type = "file";
	loading->m_name = (std::filesystem::path{ folder[0] } / name).string();
	loading->m_pos = pos;
	loading->m_progress_bar_id = progress_bar_div->add_object<divy_flex_soft_t>(depth_range_t{0.0f,0.5f}, progress_bar_div->space_x(std_margin)
	, std_margin, alignment_2D{ alignment_x::left, alignment_y::bottom }, true);
	std::unique_lock lock{ downloading_list->m_mutex };
	downloading_list->m_list.push_back(loading);
	downloading_list->m_list_process.push_back(loading);
	lock.unlock();
	return;
}

auto list(engine_t* engine, std::shared_ptr<divy_soft_t> download_progress_bar_div, std::shared_ptr<divy_soft_t> viewer_div, std::string* pos, loading_list_t* uploading_list, loading_list_t* downloading_list) -> void {
	nlohmann::json obj_list{ list_folder_content(curl_main(), pos) };
	viewer_div->clear_state();
	for (auto i : obj_list) {
		if (i["type"] == "folder") {
			viewer_div->add_object<button_text_t<text_line_scroll_std_t>>(depth_range_t{0.0f,0.5f}, std::bind(&folder_button_callback, engine, download_progress_bar_div, viewer_div, pos, i["id"], uploading_list, downloading_list)
			, "資料夾，名稱：" + i["name"].get<std::string>(), std_white, size_1D{ viewer_div->space_2D(std_margin).x });
			viewer_div->add_object<button_text_t<text_line_std_t>>(depth_range_t{0.0f,0.5f}, std::bind(&folder_download_callback, engine, download_progress_bar_div, i["name"].get<std::string>(), i["id"].get<std::string>(), uploading_list, downloading_list)
			, "下載", std_white, viewer_div->space_x(std_margin));
			viewer_div->add_object<std_linex_t>(depth_range_t{0.0f,0.5f}, viewer_div->space_x(std_margin), std_white);
		}
		else if (i["type"] == "file") {
			viewer_div->add_object<button_text_t<text_line_scroll_std_t>>(depth_range_t{0.0f,0.5f}, [] () {}
			, "檔案，名稱：" + i["name"].get<std::string>() + "，大小：" + std::to_string(i["size"].get<std::uint64_t>()), std_white, size_1D{ viewer_div->space_2D(std_margin).x });
			viewer_div->add_object<button_text_t<text_line_std_t>>(depth_range_t{0.0f,0.5f}, std::bind(&file_download_callback, engine, download_progress_bar_div, i["name"].get<std::string>(), i["id"].get<std::string>(), uploading_list, downloading_list)
			, "下載", std_white, viewer_div->space_x(std_margin));
			viewer_div->add_object<std_linex_t>(depth_range_t{0.0f,0.5f}, viewer_div->space_x(std_margin), std_white);
		}
		else {
			throw internal_error_t{ "obj type" };
		}
	}
	return;
}

auto home(engine_t* engine, state_t* state, std::shared_ptr<divy_soft_t>* upload_progress_bar_div, std::shared_ptr<divy_soft_t>* download_progress_bar_div, std::string* pos, loading_list_t* uploading_list, loading_list_t* downloading_list) -> void {
    state->clear_state();
    auto main_div{ state->add_object_shared<divy_hard_t>(depth_range_t{ 0.0f, 1.0f }, pos_2D{ 0, 0 }, engine->get_window_size()
    , size_1D{ 0 }, alignment_2D{ alignment_x::left, alignment_y::top }) };

    auto menu_left{ main_div->add_object_ghost_shared<divx_soft_t>(depth_range_t{0.0f,0.5f}, div_create_from_member{}, std_forward, std_margin
    , main_div->space_x(std_margin), alignment_2D{ alignment_x::left, alignment_y::center }) };
    menu_left->add_object<text_line_std_t>(depth_range_t{0.0f,0.5f}, "雲端硬碟", std_white, main_div->space_x(std_margin));

    auto menu_right{ main_div->add_object_shared<divx_soft_t>(depth_range_t{0.0f,0.5f}, div_create_from_member{}, std_forward, std_margin
    , main_div->space_x(std_margin), alignment_2D{ alignment_x::right, alignment_y::center }) };
    main_div->add_object_shared<std_linex_t>(depth_range_t{0.0f,0.5f}, main_div->space_x(std_margin), std_white);
    auto work_div{ main_div->add_object_shared<divx_soft_t>(depth_range_t{0.0f,0.5f}, main_div->space_2D(std_margin), std_margin, alignment_2D{ alignment_x::left, alignment_y::top }) };

    auto path_div{ work_div->add_object_ghost_shared<divy_soft_t>(depth_range_t{0.0f,0.5f}, size_2D{ 0x100, work_div->space_y(std_margin).x }
    , std_margin, alignment_2D{ alignment_x::left, alignment_y::top }) };
	*upload_progress_bar_div = work_div->add_object_ghost_shared<divy_soft_t>(depth_range_t{0.0f,0.5f}, size_2D{ 0x100, work_div->space_y(std_margin).x }
    , std_margin, alignment_2D{ alignment_x::left, alignment_y::bottom }, true);
	*download_progress_bar_div = work_div->add_object_shared<divy_soft_t>(depth_range_t{0.0f,0.5f}, size_2D{ 0x100, work_div->space_y(std_margin).x }
    , std_margin, alignment_2D{ alignment_x::left, alignment_y::top }, true);
	uploading_list->m_progress_bar_div = *upload_progress_bar_div;
	downloading_list->m_progress_bar_div = *download_progress_bar_div;
    work_div->add_object<std_liney_t>(depth_range_t{0.0f,0.5f}, work_div->space_y(std_margin), std_white);
    auto viewer_div{ work_div->add_object_shared<divy_soft_t>(depth_range_t{0.0f,0.5f}, work_div->space_2D(std_margin), std_margin, alignment_2D{ alignment_x::left, alignment_y::top }) };

	auto center_div{ state->add_object_shared<divy_hard_t>(depth_range_t{ 0.0f, 1.0f }, pos_2D{ 0, 0 }, engine->get_window_size()
    , size_1D{ 0 }, alignment_2D{ alignment_x::center, alignment_y::center }) };
	
    menu_right->add_object<button_classic_std_t>(depth_range_t{0.0f,0.5f}, std::bind(&folder_create_callback, engine, state, pos), "新增資料夾");
    menu_right->add_object<button_classic_std_t>(depth_range_t{0.0f,0.5f}, std::bind(&folder_upload_callback, engine, *upload_progress_bar_div, pos, uploading_list, downloading_list), "上傳資料夾");
    menu_right->add_object<button_classic_std_t>(depth_range_t{0.0f,0.5f}, std::bind(&file_upload_callback, engine, *upload_progress_bar_div, pos, uploading_list, downloading_list), "上傳檔案");
    menu_right->add_object<button_classic_std_t>(depth_range_t{0.0f,0.5f}, std::bind(&delete_all_callback, center_div, *upload_progress_bar_div, uploading_list), "清除");
	list(engine, *download_progress_bar_div, viewer_div, pos, uploading_list, downloading_list);
	return;
}

constexpr std::uint64_t batch_request_max{ 100 };

auto upload(loading_list_t* list, blob_empty_queue_t* empty_blob) -> void {
	curl_upload() = curl_easy_init();
	std::deque<loading_file_t> file_list{};
	std::deque<delete_file_t> delete_id{};
	while (!close_app) {
		std::unique_lock<std::mutex> lock1{ list->m_mutex };
		if (!list->m_list_process.empty()) {
			auto obj_ptr{ list->m_list_process.begin()->get() };
			auto progress_bar_div{ list->m_progress_bar_div->get_object_shared<divy_flex_soft_t>(obj_ptr->m_progress_bar_id) };
			progress_bar_div->add_object<std_linex_t>(depth_range_t{0.0f,0.5f}, progress_bar_div->space_x(std_margin), std_white);
			if (obj_ptr->m_type == "file") {
				lock1.unlock();
				curl_multipart_t multipart{ http_api_multipart(curl_upload(), account_main) };
				std::string id{ file_upload(multipart, *empty_blob, obj_ptr, nullptr, progress_bar_div
				, obj_ptr->m_pos, obj_ptr->m_name, &file_list, list->m_mutex) };
				multipart.action();
				folder_add(curl_upload(), obj_ptr->m_pos, nlohmann::json{
					{ "name", std::filesystem::path{ obj_ptr->m_name }.filename().string() },
					{ "type", "file" },
					{ "size", std::filesystem::file_size(obj_ptr->m_name) },
					{ "id", id }
				});
			}
			else if (obj_ptr->m_type == "folder") {
				auto progress_bar{ progress_bar_div->add_object_shared<text_line_scroll_std_t>(depth_range_t{0.0f,0.5f}, 
				"資料夾：0B/?B " + obj_ptr->m_name, std_blue_light, size_1D{ 0x100 }) };
				lock1.unlock();
				auto loading_folder{ std::make_shared<loading_folder_t>() };
				loading_folder->m_name = obj_ptr->m_name;
				loading_folder->m_progress_bar = progress_bar;
				curl_multipart_t multipart{ http_api_multipart(curl_upload(), account_main) };
				std::string id{ folder_upload(multipart, *empty_blob, obj_ptr, loading_folder, nullptr
				, obj_ptr->m_pos, obj_ptr->m_name, &file_list, list->m_mutex) };
				multipart.action();
				folder_add(curl_upload(), obj_ptr->m_pos, nlohmann::json{
					{ "name", std::filesystem::path{ obj_ptr->m_name }.filename().string() },
					{ "type", "folder" },
					{ "id", id }
				});
				progress_bar->set_text("資料夾：0B/" + std::to_string(obj_ptr->m_size) + "B " + obj_ptr->m_name);
			}
			else if (obj_ptr->m_type == "delete_all") {
				auto progress_bar{ progress_bar_div->add_object_shared<text_line_scroll_std_t>(depth_range_t{0.0f,0.5f}, 
				"刪除：0/? " + obj_ptr->m_name, std_blue_light, size_1D{ 0x100 }) };
				lock1.unlock();
				data_t folder_root_data{};
				folder_root_data.m_name = nlohmann::json{
					{ "name", folder_root_name },
					{ "parent", "null" },
					{ "begin", "null" },
					{ "end", "null" },
					{ "count", 0 }
				}.dump();
				data_set(curl_upload(), folder_root_id, folder_root_data);
				api_curl(curl_easy_setopt(curl_upload(), CURLOPT_HTTPGET, 1L));
				empty_blob->clear_begin();
				nlohmann::json response{ nlohmann::json::parse(http_api(curl_upload()
				, "https://www.googleapis.com/drive/v3/files", {}, {}, account_main)) };
				while (true) {
					auto& array{ response["files"] };
					for (auto i : array) {
						if (i["id"] == folder_root_id) {
							--obj_ptr->m_size;
							continue;
						}
						delete_file_t file{};
						file.m_id = i["id"];
						file.m_progress_bar = progress_bar;
						file.m_loading = obj_ptr;
						delete_id.push_back(file);
					}
					obj_ptr->m_size += array.size();
					if (!response.contains("nextPageToken")) {
						break;
					}
					response = nlohmann::json::parse(http_api(curl_upload(), "https://www.googleapis.com/drive/v3/files", {
						{ "pageToken", response["nextPageToken"] }
					}, {}, account_main));
				}
				empty_blob->clear_end();
				progress_bar->set_text("刪除：0/" + std::to_string(obj_ptr->m_size) + " " + obj_ptr->m_name);
			}
			else {
				throw;
			}
			obj_ptr->m_processed = true;
			lock1.lock();
			list->m_list_process.pop_front();
			lock1.unlock();
			continue;
		}
		lock1.unlock();
		if (!file_list.empty()) {
			loading_file_t& uploading{ file_list.front() };
			if (uploading.m_progress == uploading.m_size) {
				file_list.pop_front();
				continue;
			}
			if (uploading.m_progress < uploading.m_size) {
				std::uint64_t batch_size{ std::min(uploading.m_size - uploading.m_progress, content_size_max_base64 * (batch_request_max - 3)) };
				std::uint64_t request_count{ (batch_size + content_size_max_base64 - 1) / content_size_max_base64 };
				bool full{ uploading.m_main_last_id != "null" };
				std::uint64_t id_new_count{ full ? (request_count + 1) : request_count };
				std::vector<std::string> id{ empty_blob->get(id_new_count) };
				std::uint64_t id_i{ 0 };
				curl_multipart_t multipart{ http_api_multipart(curl_upload(), account_main) };
				std::ifstream content_stream{ create_ifstream(uploading.m_name) };
				content_stream.seekg(static_cast<std::uint64_t>(uploading.m_progress));
				nlohmann::json id_array{ nlohmann::json::array() };
				std::uint64_t id_i_copy{ full ? (id_i + 1) : id_i };
				for (std::uint64_t i{ 0 }; i < request_count; ++i) {
					id_array.push_back(id[id_i_copy]);
					++id_i_copy;
				}
				if (full) {
					data_name_set_request(multipart, uploading.m_main_last_id, nlohmann::json{
						{ "name", uploading.m_name },
						{ "parent", uploading.m_parent },
						{ "size", uploading.m_size },
						{ "last", uploading.m_main_last_last },
						{ "next", id[id_i] }
					}.dump());
					data_t main_data{};
					main_data.m_name = nlohmann::json{
						{ "name", uploading.m_name },
						{ "parent", uploading.m_parent },
						{ "size", uploading.m_size },
						{ "last", uploading.m_main_last_id },
						{ "next", "null" }
					}.dump();
					main_data.m_content = id_array.dump();
					data_set_request(multipart, id[id_i], main_data);
					uploading.m_main_last_last = uploading.m_main_last_id;
					uploading.m_main_last_id = id[id_i];
					++id_i;
				}
				else {
					data_content_set_request(multipart, uploading.m_main_id, id_array.dump());
					uploading.m_main_last_last = uploading.m_main_last_id;
					uploading.m_main_last_id = uploading.m_main_id;
				}
				if (uploading.m_last_id != "null") {
					data_name_set_request(multipart, uploading.m_last_id, nlohmann::json{
						{ "main", uploading.m_main_id },
						{ "last", uploading.m_last_last },
						{ "next", id[id_i] },
						{ "size", uploading.m_last_size }
					}.dump());
				}
				for (std::uint64_t i{ 0 }; i < request_count; ++i) {
					std::string last_id{};
					if (i) {
						last_id = id[id_i - 1];
					}
					else {
						last_id = uploading.m_last_id;
					}
					std::string next_id{};
					std::uint64_t size{};
					if (i == request_count - 1) {
						next_id = "null";
						size = batch_size - (request_count - 1) * content_size_max_base64;
						uploading.m_last_id = id[id_i];
						uploading.m_last_last = last_id;
						uploading.m_last_size = size;
					}
					else {
						next_id = id[id_i + 1];
						size = content_size_max_base64;
					}
					data_t data{};
					data.m_name = nlohmann::json{
						{ "main", uploading.m_main_id },
						{ "last", last_id },
						{ "next", next_id },
						{ "size", size }
					}.dump();
					data.m_content.resize(size);
					content_stream.read(data.m_content.data(), size);
					data.m_content = base64url_encode(data.m_content);
					data_set_request(multipart, id[id_i], data);
					++id_i;
				}
				multipart.action();
				uploading.m_progress += batch_size;
				if (uploading.m_progress_bar) {
					uploading.m_progress_bar->set_text("檔案：" + std::to_string(uploading.m_progress) + "B/" + std::to_string(uploading.m_size) + "B " + uploading.m_name);
				}
				if (uploading.m_folder) {
					uploading.m_folder->m_progress += batch_size;
					uploading.m_folder->m_progress_bar->set_text("資料夾：" + std::to_string(uploading.m_folder->m_progress) + "B/" + std::to_string(uploading.m_folder->m_size) + "B " + uploading.m_folder->m_name);
				}
				uploading.m_loading->m_progress += batch_size;
			}
		}
		else if (!delete_id.empty()) {
			std::uint64_t delete_count{ std::min(delete_id.size(), 100ULL) };
			curl_multipart_t multipart{ http_api_multipart(curl_upload(), account_main) };
			for (std::uint64_t i{ 0 }; i < delete_count; ++i) {
				http_request_t& request{ http_api_request(multipart) };
				request.method_set("DELETE");
				request.path_set("/drive/v3/files/" + delete_id[i].m_id);
			}
			multipart.action();
			for (std::uint64_t i{ 0 }; i < delete_count; ++i) {
				++delete_id[i].m_loading->m_progress;
				delete_id[i].m_progress_bar->set_text("刪除：" + std::to_string(delete_id[i].m_loading->m_progress) + "/"
				+ std::to_string(delete_id[i].m_loading->m_size) + " " + delete_id[i].m_loading->m_name);
			}
			delete_id.erase(delete_id.begin(), delete_id.begin() + delete_count);
		}
	}
	curl_easy_cleanup(curl_upload());
	return;
}

auto file_download(CURL* curl, loading_t* loading, std::shared_ptr<loading_folder_t> loading_folder, std::shared_ptr<divy_soft_t> progress_bar_div, std::string pos, std::filesystem::path file, std::deque<loading_file_t>* list, std::mutex& mutex) -> void {
	loading_file_t downloading{};
	downloading.m_name = file.string();
	if (progress_bar_div) {
		std::unique_lock<std::mutex> lock{ mutex };
		downloading.m_progress_bar = progress_bar_div->add_object_shared<text_line_scroll_std_t>(depth_range_t{0.0f,0.5f}, "檔案：0B/?B " + file.string(), std_green_light, size_1D{ 0x100 });
		lock.unlock();
	}
	downloading.m_size = nlohmann::json::parse(data_name_get(curl, pos))["size"];
	if (loading_folder) {
		loading_folder->m_size += downloading.m_size;
		downloading.m_folder = loading_folder;
	}
	loading->m_size += downloading.m_size;
	downloading.m_loading = loading;
	downloading.m_main_id = pos;
	if (progress_bar_div) {
		downloading.m_progress_bar->set_text("檔案：0B/" + std::to_string(downloading.m_size) + "B " + file.string());
	}
	list->emplace_back(downloading);
	return;
}

auto folder_download(CURL* curl, loading_t* loading, std::shared_ptr<loading_folder_t> loading_folder, std::shared_ptr<divy_soft_t> progress_bar_div, std::string pos, std::filesystem::path folder, std::deque<loading_file_t>* uploading_list, std::mutex& mutex) -> void {
	auto obj_list{ list_folder_content(curl, &pos) };
	std::filesystem::remove_all(folder);
	std::filesystem::create_directories(folder);
	for (auto i : obj_list) {
		if (i["type"] == "folder") {
			folder_download(curl, loading, loading_folder, progress_bar_div, i["id"], folder / i["name"], uploading_list, mutex);
		}
		else if (i["type"] == "file") {
			file_download(curl, loading, loading_folder, progress_bar_div, i["id"], folder / i["name"], uploading_list, mutex);
		}
		else {
			throw internal_error_t{ "obj type" };
		}
	}
	return;
}

auto download(loading_list_t* list) -> void {
	curl_download() = curl_easy_init();
	std::deque<loading_file_t> file_list{};
	while (!close_app) {
		std::unique_lock<std::mutex> lock1{ list->m_mutex };
		if (!list->m_list_process.empty()) {
			auto obj_ptr{ list->m_list_process.begin()->get() };
			auto progress_bar_div{ list->m_progress_bar_div->get_object_shared<divy_flex_soft_t>(obj_ptr->m_progress_bar_id) };
			progress_bar_div->add_object<std_linex_t>(depth_range_t{0.0f,0.5f}, progress_bar_div->space_x(std_margin), std_white);
			if (obj_ptr->m_type == "file") {
				lock1.unlock();
				file_download(curl_download(), obj_ptr, nullptr, progress_bar_div, obj_ptr->m_pos, obj_ptr->m_name, &file_list, list->m_mutex);
			}
			else if (obj_ptr->m_type == "folder") {
				auto progress_bar{ progress_bar_div->add_object_shared<text_line_scroll_std_t>(depth_range_t{0.0f,0.5f}, "資料夾：0B/?B " + obj_ptr->m_name, std_green_light, size_1D{ 0x100 }) };
				lock1.unlock();
				auto loading_folder{ std::make_shared<loading_folder_t>() };
				loading_folder->m_name = obj_ptr->m_name;
				loading_folder->m_progress_bar = progress_bar;
				folder_download(curl_download(), obj_ptr, loading_folder, nullptr, obj_ptr->m_pos, obj_ptr->m_name, &file_list, list->m_mutex);
				progress_bar->set_text("資料夾：0B/" + std::to_string(obj_ptr->m_size) + "B " + obj_ptr->m_name);
			}
			else {
				throw;
			}
			obj_ptr->m_processed = true;
			lock1.lock();
			list->m_list_process.pop_front();
			lock1.unlock();
			continue;
		}
		lock1.unlock();
		if (!file_list.empty()) {
			loading_file_t& downloading{ file_list.front() };
			if (downloading.m_progress == downloading.m_size) {
				file_list.pop_front();
				continue;
			}
			if (downloading.m_progress < downloading.m_size) {
				data_t main_data{ data_get(curl_download(), downloading.m_main_id) };
				nlohmann::json main_name{ nlohmann::json::parse(main_data.m_name) };
				nlohmann::json main_content{ nlohmann::json::parse(main_data.m_content) };
				downloading.m_main_id = main_name["next"];
				std::uint64_t request_count{ main_content.size() };
				curl_multipart_t multipart{ http_api_multipart(curl_download(), account_main) };
				for (std::uint64_t i{ 0 }; i < request_count; ++i) {
					data_download_request(multipart, main_content[i]);
				}
				multipart.action();
				std::vector<std::string> content{};
				content.resize(request_count);
				for (std::uint64_t i{ 0 }; i < request_count; ++i) {
					content[i] = nlohmann::json::parse(multipart.response_get(i).body_get())["description"];
				}
				std::ofstream content_stream{ create_ofstream_preserve(downloading.m_name) };
				content_stream.seekp(static_cast<std::uint64_t>(downloading.m_progress));
				std::uint64_t batch_size{};
				for (std::uint64_t i{ 0 }; i < request_count; ++i) {
					content[i] = base64url_decode(content[i]);
					content_stream.write(content[i].data(), content[i].size());
					batch_size += content[i].size();
				}
				downloading.m_progress += batch_size;
				if (downloading.m_progress_bar) {
					downloading.m_progress_bar->set_text("檔案：" + std::to_string(downloading.m_progress) + "B/" + std::to_string(downloading.m_size) + "B " + downloading.m_name);
				}
				if (downloading.m_folder) {
					downloading.m_folder->m_progress += batch_size;
					downloading.m_folder->m_progress_bar->set_text("資料夾：" + std::to_string(downloading.m_folder->m_progress) + "B/" + std::to_string(downloading.m_folder->m_size) + "B " + downloading.m_folder->m_name);
				}
				downloading.m_loading->m_progress += batch_size;
			}
		}
	}
	curl_easy_cleanup(curl_download());
	return;
}

auto WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) -> int {
	init_logfile("logfile" + std::to_string(std::time(nullptr)) + ".txt");
    log_file("CWD:\n");
	log_file("  " + std::filesystem::current_path().string() + "\n\n");
    init_directx();
	freetype_t freetype{};
	font_face_t::freetype_set(&freetype);
	font_face_t font_face{ "./font/NotoSansTC-VariableFont_wght.ttf", 12 };
	font_preference_t::default_get().push_back(&font_face);
    engine_t engine{ instance, size_2D{ 0x600, 0x400 }, "wWinMain.engine" };
    text_line_t::init_global(engine.device_get());
    depth_tracker_t depth_tracker{};
    state_t state{ &engine, &depth_tracker };
	loading_list_t uploading_list{}, downloading_list{};
    int argc{};
    CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc > 1) {
        log_file("參數數量不正確，不能超過0個\n");
    }
	init();
	account_main.init(
		"public@lingogoogol.iam.gserviceaccount.com",
		"-----BEGIN PRIVATE KEY-----\n"
		"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDhuOYPiqBJLtRf\n"
		"pB1z8JHFFcWf14NsUrsdzd4ZrM3T9cbC3KD5bO0rwDGOFfuwEXx1miQtKD1R9nOe\n"
		"EUetHH8oUA0sOtR7SJe+1OTou/zHLK4YckjWP6HGktMKcqT650i3kpUCyBu0eB2z\n"
		"r8rFfk6PW1cEkTe9c2e07X0V5HG8dEZ43P3bueVgNamoP5s1N9hdKFfszLlU3zt1\n"
		"to2gxizPRCWDqOx8hIn478c+GCFkpvfmkkSqMDdlHe3hJyxwCAd/E73HORnAk7ww\n"
		"GDXi24S5jZDOm3kpNK6uXSaoWSOFYbduJY6x7s48nM1F+45rdP4IOerHOOGuRNft\n"
		"CBTXIZvZAgMBAAECggEABk3B3Qq/zbNpWm9spCWCiKjCP2PHJ5IK6Drs2jY1fFcn\n"
		"kR8UvsqAOCHSsHdkM0dVvLoMixdN2RljIrW0VP0P/wn3SDYvY28ET9/Fscs4NL7G\n"
		"cOfWKWeXBsLocmj/uwMVCI1nLG/2uBxHphJBfTD5FC9Wl4nPrvKHYanlPaSG2Z6v\n"
		"PoYfMAXCFY7oIBVlbZ935HfbwoWL0DIMlruxRZ6b82bsRvnYLhTCuKKapr7V0gwT\n"
		"YklBKEcyGznNCmoe+7669JwfFFt8Y7JMVbGurx7ulBBztC/5OckbmQrs+un5iuZC\n"
		"+kwlHjZKHtmTE826u2Joks834xxDYZjR5vJL3lRP4QKBgQD5i1oRf92LuoRF5ZZK\n"
		"bglq2H+wXmUEa0lve/hcQsyJvZzA0ZfB9zgwBgrwu4zcmMiT/3Nt3rJkpCG9Bi0h\n"
		"OJJ+uYNItNs9o8IlohnnLv0clUzh38zrDFSrOPrJd9KJAKeEl1xZBZFJ9abNACaT\n"
		"bGzjPrfzipMGNTVNBWQN0SIEqQKBgQDnj8f9ggSwLvppdiYLXTVHhFgOe8tac59o\n"
		"fMNm/8nKgtzVjRbGLFkkbXIjWv9IcxCULYbFhCR/Fi4NgfX3g3MO90cjWs+cYCQA\n"
		"yck+QqBhf4yzdYxff9KRMoW2n/mMlXgpUckvv642CNSm4HyueHSdrS2GmCNJ0PEe\n"
		"4v9I2bQrsQKBgFeCHJWiZ0iGobf5KNj0+Wx0e2KX14jTLq9fo9qGuj9bNMsRNMya\n"
		"KyP8Gn4M5vq8wSnVHN9chuZwADzPG6hYtFaoK3elpHRbJswXqkm92pjZalqTJsMO\n"
		"UeqxUpCv6i6NAjGm+oVlRGvRLysYcG6mSRDvf4ZyA9jbc9OMnmeR/QHxAoGBAOO8\n"
		"Lo4N/xpfOXva7NOkWa/6Yrfr3DUbHZ2fhnWX/xeh2BpWZh2EpC8FTImlBwhz0cxJ\n"
		"chBzQ4mRgL1cIVHJ/7CqVVZaC3DZxM7ZpYgL8ruwu+oLI4EQr0CJlaPkdYNjxQeu\n"
		"HdVOCMGvsaRlh3n3kYk5Nlr26U3BpkIgMb2uDlohAoGBAOTI0j0YADYxCzUMOStD\n"
		"5IYy4ws33DUPu57NxoWVZRGaiPVAv/OmYdR++cZEAKeRNFMBLcJpKj1AFMVRAeyP\n"
		"2GVlqjPq81CsAClh+GZgRHwHyEGOo4OxecVUN7f2jW8p1tbPbc0Hd0EXfOoBwsKS\n"
		"WHB8uTlWqeZxM5LdFLOpGNr1\n"
		"-----END PRIVATE KEY-----\n"
	);
	{
		blob_empty_queue_t empty_blob{ "empty_blob", &account_main, &close_app };
		std::string pos{ folder_root_id };
		std::shared_ptr<divy_soft_t> upload_progress_bar_div{}, download_progress_bar_div{};
		home(&engine, &state, &upload_progress_bar_div, &download_progress_bar_div, &pos, &uploading_list, &downloading_list);
		std::thread thread_upload{ &upload, &uploading_list, &empty_blob }, thread_download{ &download, &downloading_list };
		while (!message_loop(&engine, &uploading_list, &downloading_list)) {}

		close_app = true;
		thread_upload.join();
		thread_download.join();
	}
	account_main.uninit();
	free();
    state.clear_state();
	engine.log_info_queue();
    log_file("finished!\n");
    return 0;
}