#define UNICODE

#include "../compilercpp/GUI/.hpp"

import std;

import storage.multipart;
import storage.api;

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
	if (!curl_main()) {
		curl_global_cleanup();
		throw error_curl_t{ "failed to create the main CURL handle" };
	}
}

auto free() -> void {
	curl_easy_cleanup(curl_main());
	curl_global_cleanup();
	return;
}

account_t account_main{};

namespace {
	constexpr std::size_t max_attempts{ 5 };

	class com_apartment_t {
	private:
		bool m_initialized{};
	public:
		com_apartment_t() {
			HRESULT result{ CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) };
			if (FAILED(result) && result != RPC_E_CHANGED_MODE) {
				throw internal_error_t{ "failed to initialize COM" };
			}
			m_initialized = SUCCEEDED(result);
		}
		~com_apartment_t() {
			if (m_initialized) {
				CoUninitialize();
			}
		}
	};

	auto path_to_utf8(const std::filesystem::path& path) -> std::string {
		auto value{ path.u8string() };
		return std::string{ reinterpret_cast<const char*>(value.data()), value.size() };
	}

	auto path_from_utf8(const std::string& value) -> std::filesystem::path {
		std::u8string utf8{};
		utf8.resize(value.size());
		std::memcpy(utf8.data(), value.data(), value.size());
		return std::filesystem::path{ utf8 };
	}

	auto safe_download_child(const std::filesystem::path& parent, const std::string& utf8_name) -> std::filesystem::path {
		const std::filesystem::path child{ path_from_utf8(utf8_name) };
		if (child.empty() || child == "." || child == ".." || child.has_root_path() || child != child.filename()) {
			throw internal_error_t{ "unsafe remote download name" };
		}
		return parent / child;
	}

	auto unique_download_path(std::filesystem::path requested, bool folder) -> std::filesystem::path {
		if (!std::filesystem::exists(requested)) {
			return requested;
		}
		const auto parent{ requested.parent_path() };
		const auto stem{ folder ? requested.filename() : requested.stem() };
		const auto extension{ folder ? std::filesystem::path{} : requested.extension() };
		for (std::uint64_t copy{ 1 }; copy != std::numeric_limits<std::uint64_t>::max(); ++copy) {
			auto candidate{ parent / (stem.native() + std::filesystem::path{ " (" + std::to_string(copy) + ")" }.native() + extension.native()) };
			if (!std::filesystem::exists(candidate)) {
				return candidate;
			}
		}
		throw internal_error_t{ "unable to choose a unique download path" };
	}

	auto retry_delay(std::size_t attempt) -> std::chrono::seconds {
		return std::chrono::seconds{ 1ULL << std::min<std::size_t>(attempt, 5) };
	}

	auto retryable_http_status(long status) -> bool {
		return status == 408 || status == 425 || status == 429 || (status >= 500 && status < 600);
	}

	auto normalize_private_key(std::string key) -> std::string {
		for (std::size_t pos{}; (pos = key.find("\\n", pos)) != std::string::npos; ++pos) {
			key.replace(pos, 2, "\n");
		}
		return key;
	}
}

auto http_api_try(CURL* curl, std::string url, std::vector<std::pair<std::string, std::string>> query
, std::vector<std::string> header, account_t& account) -> std::string {
	if (!query.empty()) {
		auto i{ query.begin() };
		url += "?" + url_encode(i->first) + "=" + url_encode(i->second);
		++i;
		for (; i != query.end(); ++i) {
			url += "&" + url_encode(i->first) + "=" + url_encode(i->second);
		}
	}
	curl_slist* header_curl{};
	header_curl = curl_slist_append(header_curl, ("Authorization: Bearer " + account.get_token()).c_str());
	for (auto i : header) {
		header_curl = curl_slist_append(header_curl, i.c_str());
	}
	std::string response{};
	try {
			api_curl(curl_easy_setopt(curl, CURLOPT_URL, url.data()));
			api_curl(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_curl));
			api_curl(curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(&response)));
			api_curl(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback));
			api_curl(curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L));
			api_curl(curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L));
			api_curl(curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 30L));
			api_curl(curl_easy_perform(curl));
	}
	catch (...) {
		curl_slist_free_all(header_curl);
		throw;
	}
	curl_slist_free_all(header_curl);
	return response;
}

auto http_api(CURL* curl, std::string url, std::vector<std::pair<std::string, std::string>> query
, std::vector<std::string> header, account_t& account) -> std::string {
	for (std::size_t attempt{}; attempt < max_attempts; ++attempt) {
		try {
			std::string result{ http_api_try(curl, url, query, header, account) };
			long response_code{};
			api_curl(curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code));
			if (response_code >= 200 && response_code < 300) {
				return result;
			}
			if (!retryable_http_status(response_code)) {
				throw error_t{ "HTTP request failed with status " + std::to_string(response_code) };
			}
			if (attempt + 1 == max_attempts) {
				throw error_t{ "HTTP request exhausted retries with status " + std::to_string(response_code) };
			}
		}
		catch (const error_curl_t& error) {
			log_file(std::string{ error.what() } + "\n");
			if (attempt + 1 == max_attempts) {
				throw;
			}
		}
		std::this_thread::sleep_for(retry_delay(attempt));
	}
	throw error_t{ "HTTP request exhausted retries" };
}

struct loading_t {
	std::string m_type{};
	std::string m_name{};
	std::atomic_uint64_t m_size{};
	std::atomic_bool m_processed{ false };
	std::atomic_bool m_failed{ false };
	std::string m_pos{};
	std::atomic_uint64_t m_progress{ 0 };
	std::uint64_t m_progress_bar_id{};
	std::shared_ptr<text_line_scroll_std_t> m_progress_text{};
	std::shared_ptr<text_line_std_t> m_progress_percent_text{};
	std::shared_ptr<rect_t> m_progress_track{};
	std::shared_ptr<rect_t> m_progress_fill{};
	std::uint64_t m_last_ui_percent{ std::numeric_limits<std::uint64_t>::max() };
	std::int64_t m_last_ui_width{ -1 };
	bool m_last_ui_failed{};
	std::optional<std::chrono::steady_clock::time_point> m_completed_at{};
};

auto mark_download_failed(loading_t* loading, const std::string& message) -> void {
	log_file("storage download failed: " + loading->m_name + ": " + message + "\n");
	loading->m_failed.store(true);
	loading->m_processed.store(true);
}

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

struct upload_transfer_progress_t {
	loading_t* m_loading{};
	std::uint64_t m_base{};
	std::uint64_t m_size{};
};

auto upload_transfer_progress(void* data, curl_off_t, curl_off_t, curl_off_t upload_total, curl_off_t upload_now) -> int {
	auto progress{ static_cast<upload_transfer_progress_t*>(data) };
	if (!progress || !progress->m_loading || upload_total <= 0 || upload_now < 0) {
		return 0;
	}
	const long double ratio{ std::min<long double>(1.0L, static_cast<long double>(upload_now) / upload_total) };
	const auto current{ progress->m_base + static_cast<std::uint64_t>(ratio * progress->m_size) };
	auto published{ progress->m_loading->m_progress.load() };
	while (published < current && !progress->m_loading->m_progress.compare_exchange_weak(published, current)) {}
	return 0;
}

class curl_upload_progress_guard_t {
private:
	CURL* m_curl{};
public:
	curl_upload_progress_guard_t(CURL* curl, upload_transfer_progress_t* progress): m_curl{ curl } {
		api_curl(curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, progress));
		api_curl(curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, &upload_transfer_progress));
		api_curl(curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L));
	}

	~curl_upload_progress_guard_t() {
		curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, nullptr);
		curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, nullptr);
	}
};

struct download_transfer_progress_t {
	loading_t* m_loading{};
	std::uint64_t m_base{};
	std::uint64_t m_size{};
};

auto download_transfer_progress(void* data, curl_off_t download_total, curl_off_t download_now, curl_off_t, curl_off_t) -> int {
	auto progress{ static_cast<download_transfer_progress_t*>(data) };
	if (!progress || !progress->m_loading || download_total <= 0 || download_now < 0) {
		return 0;
	}
	const long double ratio{ std::min<long double>(1.0L, static_cast<long double>(download_now) / download_total) };
	const auto current{ progress->m_base + static_cast<std::uint64_t>(ratio * progress->m_size) };
	auto published{ progress->m_loading->m_progress.load() };
	while (published < current && !progress->m_loading->m_progress.compare_exchange_weak(published, current)) {}
	return 0;
}

class curl_download_progress_guard_t {
private:
	CURL* m_curl{};
public:
	curl_download_progress_guard_t(CURL* curl, download_transfer_progress_t* progress): m_curl{ curl } {
		api_curl(curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, progress));
		api_curl(curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, &download_transfer_progress));
		api_curl(curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L));
	}

	~curl_download_progress_guard_t() {
		curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, nullptr);
		curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, nullptr);
	}
};

struct delete_file_t {
	std::string m_id{};
	std::shared_ptr<text_line_scroll_std_t> m_progress_bar{};
};

std::atomic_bool close_app{ false };

struct loading_list_t {
	std::deque<std::shared_ptr<loading_t>> m_list{};
	std::deque<std::shared_ptr<loading_t>> m_list_process{};
	std::shared_ptr<divy_soft_t> m_progress_bar_div{};
	std::mutex m_mutex{};
};

auto progress_label(const loading_t& loading, bool uploading) -> std::string {
	std::string action{};
	if (loading.m_type == "delete_all") {
		action = "清除";
	}
	else {
		action = uploading ? "上傳" : "下載";
		action += loading.m_type == "folder" ? "資料夾" : "檔案";
	}
	return action + "：" + loading.m_name;
}

auto refresh_progress_ui(loading_list_t* list, const std::shared_ptr<loading_t>& loading, bool uploading, color_t color) -> void {
	constexpr std::int64_t progress_height{ 0x10 };
	const std::uint64_t progress{ loading->m_progress.load() };
	const std::uint64_t size{ loading->m_size.load() };
	const bool failed{ loading->m_failed.load() };
	const bool complete{ loading->m_processed.load() && progress == size };
	const std::uint64_t percent{ size
		? std::min<std::uint64_t>(100, static_cast<std::uint64_t>((static_cast<long double>(progress) / size) * 100))
		: (complete ? 100 : 0) };
	auto progress_div{ list->m_progress_bar_div->get_object_shared<divy_flex_soft_t>(loading->m_progress_bar_id) };
	const std::int64_t progress_width{ std::max<std::int64_t>(1, progress_div->space_2D(std_margin).x) };
	if (loading->m_progress_fill && loading->m_last_ui_percent == percent && loading->m_last_ui_width == progress_width
	&& loading->m_last_ui_failed == failed) {
		return;
	}
	if (!loading->m_progress_text) {
		progress_div->add_object<std_linex_t>(depth_range_t{0.0f,0.5f}, progress_div->space_x(std_margin), std_white);
		loading->m_progress_text = progress_div->add_object_shared<text_line_scroll_std_t>(
			depth_range_t{0.0f,0.5f}, progress_label(*loading, uploading), color, size_1D{ 0x100 });
		loading->m_progress_percent_text = progress_div->add_object_shared<text_line_std_t>(
			depth_range_t{0.0f,0.5f}, "0%", color, size_1D{ 0x60 });
		loading->m_progress_track = progress_div->add_object_shared<rect_t>(
			depth_range_t{0.4f,0.5f}, size_2D{ progress_width, progress_height }, std_dark_gray);
		const pos_2D track_pos{ loading->m_progress_track->get_pos() };
		loading->m_progress_fill = progress_div->add_object_ghost_shared<rect_t>(
			depth_range_t{0.2f,0.3f}, size_2D{ 1, progress_height }, color);
		loading->m_progress_fill->set_pos(track_pos);
	}
	if (loading->m_last_ui_percent != percent || loading->m_last_ui_width != progress_width) {
		const std::int64_t fill_width{ std::max<std::int64_t>(1, progress_width * static_cast<std::int64_t>(percent) / 100) };
		loading->m_progress_track->set_size(size_2D{ progress_width, progress_height });
		loading->m_progress_fill->set_size(size_2D{ fill_width, progress_height });
	}
	if (loading->m_last_ui_percent != percent || loading->m_last_ui_failed != failed) {
		loading->m_progress_percent_text->set_text(failed ? "失敗" : std::to_string(percent) + "%");
	}
	loading->m_last_ui_percent = percent;
	loading->m_last_ui_width = progress_width;
	loading->m_last_ui_failed = failed;
}

auto message_loop(engine_t* engine, loading_list_t* uploading_list, loading_list_t* downloading_list) -> bool {
	std::unique_lock<std::mutex> upload_lock{ uploading_list->m_mutex };
	if (!uploading_list->m_list.empty()) {
		auto& uploading{ uploading_list->m_list.front() };
		refresh_progress_ui(uploading_list, uploading, true, std_blue_light);
		if (uploading->m_processed && uploading->m_progress == uploading->m_size) {
			if (!uploading->m_completed_at) {
				uploading->m_completed_at = std::chrono::steady_clock::now();
			}
			else if (std::chrono::steady_clock::now() - *uploading->m_completed_at >= std::chrono::seconds{ 1 }) {
				uploading_list->m_progress_bar_div->remove_object(uploading->m_progress_bar_id);
				uploading_list->m_list.pop_front();
			}
		}
	}
	upload_lock.unlock();
	std::unique_lock<std::mutex> download_lock{ downloading_list->m_mutex };
	if (!downloading_list->m_list.empty()) {
		auto& downloading{ downloading_list->m_list.front() };
		refresh_progress_ui(downloading_list, downloading, false, std_green_light);
		const bool failed{ downloading->m_failed.load() };
		if (failed || (downloading->m_processed && downloading->m_progress == downloading->m_size)) {
			if (!downloading->m_completed_at) {
				downloading->m_completed_at = std::chrono::steady_clock::now();
			}
			else if (std::chrono::steady_clock::now() - *downloading->m_completed_at >= std::chrono::seconds{ failed ? 5 : 1 }) {
				downloading_list->m_progress_bar_div->remove_object(downloading->m_progress_bar_id);
				downloading_list->m_list.pop_front();
			}
		}
	}
	download_lock.unlock();
	return engine->message_loop();
}

auto dialog_file(engine_t*, bool folder, loading_list_t*, loading_list_t*) -> std::vector<std::string> {
	Microsoft::WRL::ComPtr<IFileOpenDialog> dialog{};
	HRESULT result{ CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)) };
	if (FAILED(result)) {
		throw internal_error_t{ "failed to create the file-open dialog" };
	}
	FILEOPENDIALOGOPTIONS dialog_option{};
	if (FAILED(dialog->GetOptions(&dialog_option))
	|| FAILED(dialog->SetOptions(dialog_option | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT | (folder ? FOS_PICKFOLDERS : 0)))) {
		throw internal_error_t{ "failed to configure the file-open dialog" };
	}
	result = dialog->Show(nullptr);
	if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
		return {};
	}
	if (FAILED(result)) {
		throw internal_error_t{ "failed to show the file-open dialog" };
	}

	Microsoft::WRL::ComPtr<IShellItemArray> result_array{};
	if (FAILED(dialog->GetResults(&result_array))) {
		throw internal_error_t{ "failed to read file-open dialog results" };
	}
	DWORD result_count{};
	if (FAILED(result_array->GetCount(&result_count))) {
		throw internal_error_t{ "failed to count file-open dialog results" };
	}
	std::vector<std::string> paths{};
	paths.reserve(result_count);
	for (DWORD i{}; i < result_count; ++i) {
		Microsoft::WRL::ComPtr<IShellItem> item{};
		if (FAILED(result_array->GetItemAt(i, &item))) {
			throw internal_error_t{ "failed to read a file-open dialog item" };
		}
		LPWSTR path_raw{};
		if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path_raw))) {
			throw internal_error_t{ "failed to read a selected path" };
		}
		paths.push_back(to_string(path_raw));
		CoTaskMemFree(path_raw);
	}
	return paths;
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
	folder["count"] = static_cast<int>(folder["count"]) + 1;
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
	main.m_name["count"] = static_cast<int>(main.m_name["count"]) + 1;
	return;
}

auto file_upload(curl_multipart_t& multipart, blob_empty_queue_t& empty_blob, loading_t* loading
, std::shared_ptr<loading_folder_t> loading_folder, std::shared_ptr<divy_soft_t> progress_bar_div
, std::string pos, std::filesystem::path file, std::deque<loading_file_t>* list, std::mutex& mutex) -> std::string {
	loading_file_t uploading{};
	uploading.m_name = path_to_utf8(file);
	if (progress_bar_div) {
		std::unique_lock<std::mutex> lock{ mutex };
		uploading.m_progress_bar = progress_bar_div->add_object_shared<text_line_scroll_std_t>(depth_range_t{0.0f,0.0f}, "檔案：0B/?B " + path_to_utf8(file), std_blue_light, size_1D{ 0x100 });
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
		{ "name", path_to_utf8(file.filename()) },
		{ "parent", pos },
		{ "size", uploading.m_size },
		{ "last", "null" },
		{ "next", "null" }
	}.dump();
	data.m_content = nlohmann::json::array().dump();
	data_set_request(multipart, uploading.m_main_id, data);
	http_api_request_action(multipart);
	if (progress_bar_div) {
		uploading.m_progress_bar->set_text("檔案：0B/" + std::to_string(uploading.m_size) + "B " + path_to_utf8(file));
	}
	list->emplace_back(uploading);
	log_file(path_to_utf8(file) + "\n");
	return uploading.m_main_id;
}

auto folder_upload(curl_multipart_t& multipart, blob_empty_queue_t& empty_blob, loading_t* loading
, std::shared_ptr<loading_folder_t> loading_folder, std::shared_ptr<divy_soft_t> progress_bar_div
, std::string pos, std::filesystem::path folder, std::deque<loading_file_t>* uploading_list, std::mutex& mutex) -> std::string {
	blob_t main{};
	main.m_id = empty_blob.get();
	main.m_name = nlohmann::json{
		{ "name", path_to_utf8(folder.filename()) },
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
				{ "name", path_to_utf8(j->path().filename()) },
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
	log_file(path_to_utf8(folder) + "\n");
	return main.m_id;
}

auto enqueue_upload(std::string type, std::string path, std::shared_ptr<divy_soft_t> progress_bar_div, const std::string& pos, loading_list_t* uploading_list) -> void {
		auto loading{ std::make_shared<loading_t>() };
		loading->m_type = std::move(type);
		loading->m_name = std::move(path);
		loading->m_pos = pos;
		loading->m_progress_bar_id = progress_bar_div->add_object<divy_flex_soft_t>(depth_range_t{0.0f,0.5f}, progress_bar_div->space_x(std_margin)
    	, std_margin, alignment_2D{ alignment_x::left, alignment_y::bottom }, true);
		std::unique_lock lock{ uploading_list->m_mutex };
		uploading_list->m_list.push_back(loading);
		uploading_list->m_list_process.push_back(loading);
	return;
}

auto folder_upload_callback(engine_t* engine, std::shared_ptr<divy_soft_t> progress_bar_div, std::string* pos, loading_list_t* uploading_list, loading_list_t* downloading_list) -> void {
	std::vector<std::string> folder{ dialog_file(engine, true, uploading_list, downloading_list) };
	for (auto& path : folder) {
		enqueue_upload("folder", std::move(path), progress_bar_div, *pos, uploading_list);
	}
	return;
}

auto file_upload_callback(engine_t* engine, std::shared_ptr<divy_soft_t> progress_bar_div, std::string* pos, loading_list_t* uploading_list, loading_list_t* downloading_list) -> void {
	std::vector<std::string> file{ dialog_file(engine, false, uploading_list, downloading_list) };
	for (auto& path : file) {
		enqueue_upload("file", std::move(path), progress_bar_div, *pos, uploading_list);
	}
	return;
}

auto list(engine_t* engine, std::shared_ptr<divy_soft_t> download_progress_bar_div, std::shared_ptr<divy_soft_t> viewer_div, std::string* pos, loading_list_t* uploading_list, loading_list_t* downloading_list) -> void;

auto delete_all_confirm_callback(std::shared_ptr<divy_hard_t> center_div, std::shared_ptr<divy_soft_t> progress_bar_div, std::shared_ptr<text_line_std_input_t> input, loading_list_t* uploading_list) {
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
	input_div->add_object<text_line_flex_std_t>(depth_range_t{0.0f,0.0f}, "輸入框（點選下方空白處後輸入\"清除\"）：", size_1D{ 0x180 }, std_red);
	auto focus{ new focus_t{} };
	auto input{ input_div->add_object_shared<text_line_std_input_t>(depth_range_t{0.0f,0.0f}, "", focus, std_red, size_1D{ 0x180 }) };
	input_div->add_object<std_linex_t>(depth_range_t{0.0f,0.5f}, size_1D{ 0x180 }, std_yellow);
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
		return;
	}
	auto loading{ std::make_shared<loading_t>() };
	loading->m_type = "folder";
	loading->m_name = path_to_utf8(unique_download_path(safe_download_child(path_from_utf8(folder[0]), name), true));
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
		return;
	}
	auto loading{ std::make_shared<loading_t>() };
	loading->m_type = "file";
	loading->m_name = path_to_utf8(unique_download_path(safe_download_child(path_from_utf8(folder[0]), name), false));
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
	if (!curl_upload()) {
		log_file("failed to create the upload CURL handle\n");
		return;
	}
	std::deque<loading_file_t> file_list{};
	std::deque<delete_file_t> delete_id{};
	while (!close_app) {
		std::unique_lock<std::mutex> lock1{ list->m_mutex };
		if (!list->m_list_process.empty()) {
			auto obj_ptr{ list->m_list_process.begin()->get() };
			auto local_path{ path_from_utf8(obj_ptr->m_name) };
			if (obj_ptr->m_type == "file") {
				lock1.unlock();
				curl_multipart_t multipart{ http_api_multipart(curl_upload(), account_main) };
				std::string id{ file_upload(multipart, *empty_blob, obj_ptr, nullptr, nullptr
				, obj_ptr->m_pos, local_path, &file_list, list->m_mutex) };
				multipart.action();
				folder_add(curl_upload(), obj_ptr->m_pos, nlohmann::json{
					{ "name", path_to_utf8(local_path.filename()) },
					{ "type", "file" },
					{ "size", std::filesystem::file_size(local_path) },
					{ "id", id }
				});
			}
			else if (obj_ptr->m_type == "folder") {
				lock1.unlock();
				auto loading_folder{ std::make_shared<loading_folder_t>() };
				loading_folder->m_name = obj_ptr->m_name;
				curl_multipart_t multipart{ http_api_multipart(curl_upload(), account_main) };
				std::string id{ folder_upload(multipart, *empty_blob, obj_ptr, loading_folder, nullptr
				, obj_ptr->m_pos, local_path, &file_list, list->m_mutex) };
				multipart.action();
				folder_add(curl_upload(), obj_ptr->m_pos, nlohmann::json{
					{ "name", path_to_utf8(local_path.filename()) },
					{ "type", "folder" },
					{ "id", id }
				});
			}
			else if (obj_ptr->m_type == "delete_all") {
				lock1.unlock();
				data_t folder_root_data{};
				folder_root_data.m_name = nlohmann::json{
					{ "name", folder_root_name },
					{ "parent", "null" },
					{ "begin", "null" },
					{ "end", "null" },
					{ "count", 0 }
				}.dump();
				obj_ptr->m_size.store(1);
				data_set(curl_upload(), folder_root_id, folder_root_data);
				obj_ptr->m_progress.store(1);
				obj_ptr->m_processed.store(true);
				api_curl(curl_easy_setopt(curl_upload(), CURLOPT_HTTPGET, 1L));
				empty_blob->clear_begin();
				nlohmann::json response{ nlohmann::json::parse(http_api(curl_upload()
				, "https://www.googleapis.com/drive/v3/files", {}, {}, account_main)) };
				while (true) {
					auto& array{ response["files"] };
					for (auto i : array) {
						if (i["id"] == folder_root_id) {
							continue;
						}
						delete_file_t file{};
						file.m_id = i["id"];
						delete_id.push_back(file);
					}
					if (!response.contains("nextPageToken")) {
						break;
					}
					response = nlohmann::json::parse(http_api(curl_upload(), "https://www.googleapis.com/drive/v3/files", {
						{ "pageToken", response["nextPageToken"] }
					}, {}, account_main));
				}
				empty_blob->clear_end();
			}
			else {
				throw internal_error_t{ "unknown upload work-item type" };
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
				std::ifstream content_stream{ create_ifstream(path_from_utf8(uploading.m_name)) };
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
				upload_transfer_progress_t transfer_progress{
					uploading.m_loading, uploading.m_loading->m_progress.load(), batch_size
				};
				curl_upload_progress_guard_t transfer_progress_guard{ curl_upload(), &transfer_progress };
				multipart.action();
				uploading.m_progress += batch_size;
				if (uploading.m_folder) {
					uploading.m_folder->m_progress += batch_size;
				}
				uploading.m_loading->m_progress.store(transfer_progress.m_base + batch_size);
			}
		}
		else if (!delete_id.empty()) {
			constexpr std::uint64_t delete_batch_max{ 100 };
			const std::uint64_t delete_count{ std::min<std::uint64_t>({
				static_cast<std::uint64_t>(delete_id.size()), delete_batch_max
			}) };
			curl_multipart_t multipart{ http_api_multipart(curl_upload(), account_main) };
			for (std::uint64_t i{ 0 }; i < delete_count; ++i) {
				http_request_t& request{ http_api_request(multipart) };
				request.method_set("DELETE");
				request.path_set("/drive/v3/files/" + delete_id[i].m_id);
			}
			multipart.action();
			delete_id.erase(delete_id.begin(), delete_id.begin() + delete_count);
		}
	}
	curl_easy_cleanup(curl_upload());
	return;
}

auto file_download(CURL* curl, loading_t* loading, std::shared_ptr<loading_folder_t> loading_folder, std::shared_ptr<divy_soft_t> progress_bar_div, std::string pos, std::filesystem::path file, std::deque<loading_file_t>* list, std::mutex& mutex) -> void {
	loading_file_t downloading{};
	downloading.m_name = path_to_utf8(file);
	downloading.m_size = nlohmann::json::parse(data_name_get(curl, pos))["size"];
	create_ofstream_destroy(path_from_utf8(downloading.m_name));
	if (loading_folder) {
		loading_folder->m_size += downloading.m_size;
		downloading.m_folder = loading_folder;
	}
	loading->m_size += downloading.m_size;
	downloading.m_loading = loading;
	downloading.m_main_id = pos;
	list->emplace_back(downloading);
	return;
}

auto folder_download(CURL* curl, loading_t* loading, std::shared_ptr<loading_folder_t> loading_folder, std::shared_ptr<divy_soft_t> progress_bar_div, std::string pos, std::filesystem::path folder, std::deque<loading_file_t>* uploading_list, std::mutex& mutex) -> void {
	auto obj_list{ list_folder_content(curl, &pos) };
	std::filesystem::create_directories(folder);
	for (auto i : obj_list) {
		if (i["type"] == "folder") {
			folder_download(curl, loading, loading_folder, progress_bar_div, i["id"], unique_download_path(safe_download_child(folder, i["name"].get<std::string>()), true), uploading_list, mutex);
		}
		else if (i["type"] == "file") {
			file_download(curl, loading, loading_folder, progress_bar_div, i["id"], unique_download_path(safe_download_child(folder, i["name"].get<std::string>()), false), uploading_list, mutex);
		}
		else {
			throw internal_error_t{ "obj type" };
		}
	}
	return;
}

auto download(loading_list_t* list) -> void {
	curl_download() = curl_easy_init();
	if (!curl_download()) {
		log_file("failed to create the download CURL handle\n");
		return;
	}
	std::deque<loading_file_t> file_list{};
	while (!close_app) {
		std::unique_lock<std::mutex> lock1{ list->m_mutex };
		if (!list->m_list_process.empty()) {
			auto obj{ list->m_list_process.front() };
			auto obj_ptr{ obj.get() };
			lock1.unlock();
			try {
				if (obj_ptr->m_type == "file") {
					file_download(curl_download(), obj_ptr, nullptr, nullptr, obj_ptr->m_pos, obj_ptr->m_name, &file_list, list->m_mutex);
				}
				else if (obj_ptr->m_type == "folder") {
					auto loading_folder{ std::make_shared<loading_folder_t>() };
					loading_folder->m_name = obj_ptr->m_name;
					folder_download(curl_download(), obj_ptr, loading_folder, nullptr, obj_ptr->m_pos, obj_ptr->m_name, &file_list, list->m_mutex);
				}
				else {
					throw internal_error_t{ "unknown download work-item type" };
				}
				obj_ptr->m_processed.store(true);
			}
			catch (const std::exception& error) {
				mark_download_failed(obj_ptr, error.what());
				std::erase_if(file_list, [obj_ptr](const loading_file_t& file) { return file.m_loading == obj_ptr; });
			}
			catch (...) {
				mark_download_failed(obj_ptr, "unknown error");
				std::erase_if(file_list, [obj_ptr](const loading_file_t& file) { return file.m_loading == obj_ptr; });
			}
			lock1.lock();
			if (!list->m_list_process.empty() && list->m_list_process.front() == obj) {
				list->m_list_process.pop_front();
			}
			lock1.unlock();
			continue;
		}
		lock1.unlock();
		if (!file_list.empty()) {
			loading_t* failed_loading{ file_list.front().m_loading };
			try {
				loading_file_t& downloading{ file_list.front() };
				if (downloading.m_progress == downloading.m_size) {
					file_list.pop_front();
					continue;
				}
				if (downloading.m_progress >= downloading.m_size) {
					throw internal_error_t{ "download progress exceeds declared file size" };
				}
				if (downloading.m_main_id == "null") {
					throw internal_error_t{ "download chain ended before declared file size" };
				}
				data_t main_data{ data_get(curl_download(), downloading.m_main_id) };
				nlohmann::json main_name{ nlohmann::json::parse(main_data.m_name) };
				nlohmann::json main_content{ nlohmann::json::parse(main_data.m_content) };
				if (!main_content.is_array() || main_content.empty()) {
					throw internal_error_t{ "download batch contains no data" };
				}
				std::uint64_t request_count{ main_content.size() };
				if (request_count > batch_request_max) {
					throw internal_error_t{ "download batch contains too many requests" };
				}
				const std::uint64_t remaining{ downloading.m_size - downloading.m_progress };
				const std::uint64_t expected_batch_size{ std::min(remaining, request_count * content_size_max_base64) };
				download_transfer_progress_t transfer_progress{
					downloading.m_loading, downloading.m_loading->m_progress.load(), expected_batch_size
				};
				curl_multipart_t multipart{ http_api_multipart(curl_download(), account_main) };
				for (std::uint64_t i{ 0 }; i < request_count; ++i) {
					data_download_request(multipart, main_content[i]);
				}
				curl_download_progress_guard_t transfer_progress_guard{ curl_download(), &transfer_progress };
				multipart.action();
				std::vector<std::string> content{};
				content.resize(request_count);
				std::uint64_t batch_size{};
				for (std::uint64_t i{ 0 }; i < request_count; ++i) {
					content[i] = nlohmann::json::parse(multipart.response_get(i).body_get())["description"];
					content[i] = base64url_decode(content[i]);
					batch_size += content[i].size();
				}
				if (batch_size == 0) {
					throw internal_error_t{ "download batch contains no data" };
				}
				if (batch_size > remaining) {
					throw internal_error_t{ "download exceeds declared file size" };
				}
				std::ofstream content_stream{ create_ofstream_preserve(path_from_utf8(downloading.m_name)) };
				content_stream.seekp(static_cast<std::uint64_t>(downloading.m_progress));
				for (std::uint64_t i{ 0 }; i < request_count; ++i) {
					content_stream.write(content[i].data(), content[i].size());
				}
				downloading.m_main_id = main_name["next"].get<std::string>();
				downloading.m_progress += batch_size;
				if (downloading.m_folder) {
					downloading.m_folder->m_progress += batch_size;
				}
				downloading.m_loading->m_progress.store(transfer_progress.m_base + batch_size);
			}
			catch (const std::exception& error) {
				mark_download_failed(failed_loading, error.what());
				std::erase_if(file_list, [failed_loading](const loading_file_t& file) { return file.m_loading == failed_loading; });
			}
			catch (...) {
				mark_download_failed(failed_loading, "unknown error");
				std::erase_if(file_list, [failed_loading](const loading_file_t& file) { return file.m_loading == failed_loading; });
			}
		}
	}
	curl_easy_cleanup(curl_download());
	return;
}

auto upload_guard(loading_list_t* list, blob_empty_queue_t* empty_blob) -> void {
	try {
		upload(list, empty_blob);
	}
	catch (const std::exception& error) {
		log_file(std::string{ "storage worker failed: upload: " } + error.what() + "\n");
	}
	catch (...) {
		log_file("storage worker failed: upload: unknown error\n");
	}
}

auto download_guard(loading_list_t* list) -> void {
	try {
		download(list);
	}
	catch (const std::exception& error) {
		log_file(std::string{ "storage worker failed: download: " } + error.what() + "\n");
	}
	catch (...) {
		log_file("storage worker failed: download: unknown error\n");
	}
}

auto WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) -> int {
	init_logfile("logfile" + std::to_string(std::time(nullptr)) + ".txt");
    log_file("CWD:\n");
	log_file("  " + std::filesystem::current_path().string() + "\n\n");
	const char* service_account_email{ std::getenv("LINGOGOOGOL_SERVICE_ACCOUNT_EMAIL") };
	const char* service_account_private_key{ std::getenv("LINGOGOOGOL_SERVICE_ACCOUNT_PRIVATE_KEY") };
	if (!service_account_email || !*service_account_email || !service_account_private_key || !*service_account_private_key) {
		log_file("Missing LINGOGOOGOL_SERVICE_ACCOUNT_EMAIL or LINGOGOOGOL_SERVICE_ACCOUNT_PRIVATE_KEY.\n");
		return EXIT_FAILURE;
	}
	com_apartment_t com_apartment{};
    init_directx();
	freetype_t freetype{};
	font_face_t::freetype_set(&freetype);
	const std::filesystem::path bundled_font{ "./font/NotoSansTC-VariableFont_wght.ttf" };
	const std::filesystem::path font_path{ std::filesystem::exists(bundled_font)
		? bundled_font : std::filesystem::path{ "C:/Windows/Fonts/msjh.ttc" } };
	font_face_t font_face{ font_path.string(), static_cast<std::uint64_t>(std_font_size.x) };
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
	account_main.init(service_account_email, normalize_private_key(service_account_private_key));
	{
		blob_empty_queue_t empty_blob{ "empty_blob", &account_main, &close_app };
		std::string pos{ folder_root_id };
		std::shared_ptr<divy_soft_t> upload_progress_bar_div{}, download_progress_bar_div{};
		home(&engine, &state, &upload_progress_bar_div, &download_progress_bar_div, &pos, &uploading_list, &downloading_list);
		std::thread thread_upload{ &upload_guard, &uploading_list, &empty_blob }, thread_download{ &download_guard, &downloading_list };
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
