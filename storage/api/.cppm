module;

#define UNICODE

#include "../../compilercpp/lib/.hpp"

export module storage.api;

import std;

import storage.multipart;

export auto http_post_simple(CURL* curl, std::string url, std::string body) -> std::string {
	std::string response{};
    while (true) {
        try {
			api_curl(curl_easy_setopt(curl, CURLOPT_URL, url.data()));
			api_curl(curl_easy_setopt(curl, CURLOPT_POST, 1L));
			api_curl(curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size())));
			api_curl(curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, body.data()));
			api_curl(curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(&response)));
			api_curl(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback));
			api_curl(curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L));
			api_curl(curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L));
			api_curl(curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 30L));
			api_curl(curl_easy_perform(curl));
		}
        catch (error_curl_t error) {
            log_file(error.what() + "\n");
            continue;
        }
        break;
    }
	return response;
}

export auto base64url_encode(std::string in) -> std::string {
	DWORD out_size{};
	CryptBinaryToStringA(reinterpret_cast<const BYTE*>(in.data()), static_cast<DWORD>(in.size())
	, CRYPT_STRING_BASE64URI | CRYPT_STRING_NOCRLF, nullptr, &out_size);
	std::string out{};
	out.resize(out_size);
	CryptBinaryToStringA(reinterpret_cast<const BYTE*>(in.data()), static_cast<DWORD>(in.size())
	, CRYPT_STRING_BASE64URI | CRYPT_STRING_NOCRLF, out.data(), &out_size);
	out.pop_back();
	return out;
}

export auto base64url_decode(std::string in) -> std::string {
	DWORD out_size{};
	CryptStringToBinaryA(reinterpret_cast<LPCSTR>(in.data()), static_cast<DWORD>(in.size())
	, CRYPT_STRING_BASE64, nullptr, &out_size, 0, nullptr);
	std::string out{};
	out.resize(out_size);
	CryptStringToBinaryA(reinterpret_cast<LPCSTR>(in.data()), static_cast<DWORD>(in.size())
	, CRYPT_STRING_BASE64, reinterpret_cast<BYTE*>(out.data()), &out_size, 0, nullptr);
	return out;
}

export auto sha256withrsa(std::string in, std::string key) -> std::string {
	EVP_PKEY* private_key{};
	OSSL_DECODER_CTX* ossl_decoder{
		OSSL_DECODER_CTX_new_for_pkey(&private_key, "PEM", nullptr, "RSA", EVP_PKEY_KEYPAIR, nullptr, nullptr)
	};
	auto pem_data{ reinterpret_cast<const unsigned char*>(key.data()) };
	auto pem_size{ key.size() };
	OSSL_DECODER_from_data(ossl_decoder, &pem_data, &pem_size);
	EVP_PKEY_CTX* openssl_pkey{ EVP_PKEY_CTX_new(private_key, nullptr) };
	EVP_SIGNATURE* sign_algo{ EVP_SIGNATURE_fetch(nullptr, "RSA-SHA256", nullptr) };
	EVP_PKEY_sign_message_init(openssl_pkey, sign_algo, nullptr);
	EVP_PKEY_sign_message_update(openssl_pkey, reinterpret_cast<const unsigned char*>(in.data()), in.size());
	std::size_t out_size{};
	EVP_PKEY_sign_message_final(openssl_pkey, nullptr, &out_size);
	std::string out{};
	out.resize(out_size);
	EVP_PKEY_sign_message_final(openssl_pkey, reinterpret_cast<unsigned char*>(out.data()), &out_size);
	return out;
}

export class account_t {
private:
	std::string m_address{};
	std::string m_key{};
	std::string m_token{};
	std::time_t m_token_life{};
	CURL* m_curl{};
	std::mutex m_mutex{};
public:
	account_t() = default;
	account_t(std::string address, std::string key);
	~account_t();
	auto init(std::string address, std::string key) -> void;
	auto uninit() -> void;

	auto get_token() -> std::string;
};

account_t::account_t(std::string address, std::string key): m_address{ address }, m_key{ key } {
	m_curl = curl_easy_init();
	return;
}

account_t::~account_t() {
	if (m_curl) {
		uninit();
	}
	return;
}

auto account_t::init(std::string address, std::string key) -> void {
	m_address = address;
	m_key = key;
	m_curl = curl_easy_init();
	return;
}

auto account_t::uninit() -> void {
	curl_easy_cleanup(m_curl);
	m_curl = nullptr;
	return;
}

auto account_t::get_token() -> std::string {
	std::unique_lock lock{ m_mutex };
	std::time_t time{ std::time(nullptr) };
	if (time >= m_token_life) {
		std::string encoded{
			base64url_encode(
				"{"
					"\"alg\":\"RS256\","
					"\"typ\":\"JWT\""
				"}"
			) + "." + base64url_encode(
				"{"
					"\"iss\":\"" + m_address + "\","
					"\"scope\":\"https://www.googleapis.com/auth/drive\","
					"\"aud\":\"https://oauth2.googleapis.com/token\","
					"\"exp\":" + std::to_string(time + 3600) + ","
					"\"iat\":" + std::to_string(time) +
				"}"
			)
		};
		std::string body{
			"grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion="
			+ encoded + "." + base64url_encode(sha256withrsa(encoded, m_key))
		};
		std::string response{ http_post_simple(m_curl, "https://oauth2.googleapis.com/token", body) };
		m_token = nlohmann::json::parse(response)["access_token"].get<std::string>();
		m_token_life = time + 3600 / 2;
	}
	return m_token;
}

export struct data_t {
	std::string m_name{};
	std::string m_content{};
};

export auto http_api_multipart(CURL* curl, account_t& account) -> curl_multipart_t {
	curl_multipart_t multipart{ curl };
	multipart.host_set("https://www.googleapis.com");
	multipart.path_set("/batch/drive/v3");
	multipart.header_set("Authorization", "Bearer " + account.get_token());
	multipart.boundary_set("+");
	return multipart;
}

export auto http_api_request(curl_multipart_t& multipart) -> http_request_t& {
	http_request_t& request{ multipart.request_get(multipart.request_add()) };
	request.host_set("https://www.googleapis.com");
	return request;
}

export auto http_api_request_action(curl_multipart_t& multipart) -> void {
	if (multipart.size() >= 100) {
		multipart.action();
		multipart.clear();
	}
	return;
}

export auto data_upload_request(curl_multipart_t& multipart, std::string name, std::string content) -> void {
	http_request_t& request{ http_api_request(multipart) };
	request.method_set("POST");
	request.path_set("/drive/v3/files");
	request.query_set("fields", "id");
	request.header_set("Content-Type", "application/json; charset=UTF-8");
	request.body_set(nlohmann::json{
		{ "name", name },
		{ "description", content }
	}.dump());
	return;
}

export auto data_update_request(curl_multipart_t& multipart, std::string id, std::string body) -> void {
	http_request_t& request{ http_api_request(multipart) };
	request.method_set("PATCH");
	request.path_set("/drive/v3/files/" + id);
	request.body_set(body);
	return;
}

export auto data_set_request(curl_multipart_t& multipart, std::string id, data_t data) -> void {
	data_update_request(multipart, id, nlohmann::json{
		{ "name", data.m_name },
		{ "description", data.m_content }
	}.dump());
	return;
}

export struct blob_t {
	std::string m_id{};
	nlohmann::json m_name{};
	nlohmann::json m_content{};
};

export auto data_set_request(curl_multipart_t& multipart, blob_t blob) -> void {
	data_t data{};
	data.m_name = blob.m_name.dump();
	data.m_content = blob.m_content.dump();
	data_set_request(multipart, blob.m_id, data);
	return;
}

export auto data_name_set_request(curl_multipart_t& multipart, std::string id, std::string name) -> void {
	data_update_request(multipart, id, nlohmann::json{
		{ "name", name }
	}.dump());
	return;
}

export auto data_content_set_request(curl_multipart_t& multipart, std::string id, std::string content) -> void {
	data_update_request(multipart, id, nlohmann::json{
		{ "description", content }
	}.dump());
	return;
}

export auto data_download_request(curl_multipart_t& multipart, std::string id) -> void {
	http_request_t& request{ http_api_request(multipart) };
	request.method_set("GET");
	request.path_set("/drive/v3/files/" + id);
	request.query_set("fields", "description");
	return;
}

export class blob_empty_queue_t {
private:
	std::deque<std::string> m_id{};
	std::mutex m_mutex{};
	std::thread m_thread{};
	std::condition_variable m_condition_variable{};
	CURL* m_curl{};
	std::filesystem::path m_path{};
	account_t* m_account{};
	const std::atomic_bool* m_close{};
	bool m_clearing{ false };
	bool m_cleared{ false };

	auto process() -> void {
		while (!*m_close) {
			std::unique_lock lock{ m_mutex };
			if (m_id.size() >= 200) {
				continue;
			}
			bool clear_copy{ m_cleared };
			lock.unlock();
			curl_multipart_t multipart{ http_api_multipart(m_curl, *m_account) };
			for (std::uint64_t i{ 0 }; i < 100; ++i) {
				data_upload_request(multipart, "", "");
			}
			multipart.action();
			lock.lock();
			if (!m_cleared || clear_copy) {
				std::uint64_t size_orig{ m_id.size() };
				m_id.resize(size_orig + 100);
				for (std::uint64_t i{ 0 }; i < 100; ++i) {
					m_id[size_orig + i] = nlohmann::json::parse(multipart.response_get(i).body_get())["id"];
				}
				lock.unlock();
				m_condition_variable.notify_all();
			}
			m_cleared = false;
		}
		return;
	}
public:
	blob_empty_queue_t(std::filesystem::path path, account_t* account, const std::atomic_bool* close)
	: m_curl{ curl_easy_init() }, m_path{ path }, m_account{ account }, m_close{ close } {
		if (std::filesystem::is_regular_file(path)) {
			std::uint64_t size{ std::filesystem::file_size(path) };
			std::string content{};
			content.resize(size);
			create_ifstream(path.string()).read(content.data(), size);
			nlohmann::json arr{ nlohmann::json::parse(content) };
			m_id.resize(arr.size());
			for (std::uint64_t i{ 0 }; i < arr.size(); ++i) {
				m_id[i] = arr[i];
			}
		}
		m_thread = std::thread{ std::bind(&blob_empty_queue_t::process, this) };
		return;
	}

	~blob_empty_queue_t() {
		m_thread.join();
		curl_easy_cleanup(m_curl);
		nlohmann::json arr{ nlohmann::json::array() };
		for (std::uint64_t i{ 0 }; i < m_id.size(); ++i) {
			arr.push_back(m_id[i]);
		}
		std::string content{ arr.dump() };
		create_ofstream_destroy(m_path.string()).write(content.data(), content.size());
		return;
	}

	auto get() -> std::string {
		std::unique_lock lock{ m_mutex };
		m_condition_variable.wait(lock, [this] () -> bool { return !m_clearing && m_id.size() > 1; });
		std::string out{ m_id.front() };
		m_id.pop_front();
		return out;
	}

	auto get(std::uint64_t count) -> std::vector<std::string> {
		std::unique_lock lock{ m_mutex };
		m_condition_variable.wait(lock, [this, count] () -> bool { return !m_clearing && m_id.size() >= count; });
		std::vector<std::string> out{ m_id.begin(), m_id.begin() + count };
		m_id.erase(m_id.begin(), m_id.begin() + count);
		return out;
	}

	auto clear_begin() -> void {
		std::unique_lock lock{ m_mutex };
		m_clearing = true;
		return;
	}

	auto clear_end() -> void {
		std::unique_lock lock{ m_mutex };
		m_id.clear();
		m_cleared = true;
		m_clearing = false;
		lock.unlock();
		m_condition_variable.notify_all();
		return;
	}
};
