export module storage.multipart;

import std;

export class error_curl_t: public error_t {
public:
	error_curl_t(std::string message): error_t{ message } {}
};

export auto api_curl(CURLcode code) -> void {
	if (code != CURLE_OK) {
		throw error_curl_t{ curl_easy_strerror(code) };
	}
	return;
}

export auto api_curlh(CURLHcode code) -> void {
	if (code != CURLHE_OK) {
		throw error_curl_t{ "error curlh: " + std::to_string(code) };
	}
	return;
}

export auto write_callback(char* src, std::size_t, std::size_t size, void* dest_void) -> std::size_t {
	auto dest{ static_cast<std::string*>(dest_void) };
	auto size_orig{ dest->size() };
	dest->resize(size_orig + size);
	for (std::size_t i{ 0 }; i < size; ++i) {
		(*dest)[size_orig + i] = src[i];
	}
	return size;
}

export struct read_src_t {
	std::string m_data{};
	std::uint64_t m_progress{};
};

export auto read_callback(char* dest, std::size_t, std::size_t size, void* src_void) -> std::size_t {
	auto src{ static_cast<read_src_t*>(src_void) };
	size = std::min(size, src->m_data.size() - src->m_progress);
	for (std::size_t i{ 0 }; i < size; ++i) {
		dest[i] = src->m_data[src->m_progress + i];
	}
	src->m_progress += size;
	return size;
}

export struct debug_str_t {
	std::string m_text{};
	std::string m_request_header{};
};

export auto debug_callback(CURL*, curl_infotype type, char* data, std::size_t size, void* str_void) -> int {
	auto str_list{ static_cast<debug_str_t*>(str_void) };
	std::string* str{};
	switch (type) {
	case CURLINFO_TEXT: {
		str = &str_list->m_text;
	}
	case CURLINFO_HEADER_OUT: {
		str = &str_list->m_request_header;
	}
	default: {}
	}
	if (str) {
		for (std::size_t i{ 0 }; i < size; ++i) {
			str->push_back(data[i]);
		}
	}
	return 0;
}

export class http_request_t {
private:
    std::string m_method{};
    std::string m_path{};
    std::string m_host{};
    std::map<std::string, std::string> m_query{};
    std::map<std::string, std::string> m_header{};
    std::string m_body{};
public:
    http_request_t() = default;

    auto method_get() const -> std::string;
    auto method_set(std::string method) -> void;
    auto host_get() const -> std::string;
    auto host_set(std::string host) -> void;
    auto path_get() const -> std::string;
    auto path_set(std::string path) -> void;
    auto query_get(std::string key) const -> std::string;
    auto query_set(std::string key, std::string val) -> void;
    auto header_get(std::string name) const -> std::string;
    auto header_set(std::string name, std::string val) -> void;
    auto body_get() const -> std::string;
    auto body_set(std::string body) -> void;
    auto to_str() const -> std::string;
};

auto http_request_t::method_get() const -> std::string {
    return m_method;
}

auto http_request_t::method_set(std::string method) -> void {
    m_method = method;
    return;
}

auto http_request_t::host_get() const -> std::string {
    return m_host;
}

auto http_request_t::host_set(std::string host) -> void {
    m_host = host;
    return;
}

auto http_request_t::path_get() const -> std::string {
    return m_path;
}

auto http_request_t::path_set(std::string path) -> void {
    m_path = path;
    return;
}

auto http_request_t::query_get(std::string key) const -> std::string {
    return m_query.at(key);
}

auto http_request_t::query_set(std::string key, std::string val) -> void {
    m_query[key] = val;
    return;
}

auto http_request_t::header_get(std::string name) const -> std::string {
    return m_header.at(name);
}

auto http_request_t::header_set(std::string name, std::string val) -> void {
    m_header[name] = val;
    return;
}

auto http_request_t::body_get() const -> std::string {
    return m_body;
}

auto http_request_t::body_set(std::string body) -> void {
    m_body = body;
    return;
}

auto http_request_t::to_str() const -> std::string {
    std::string out{};
    out += m_method + " " + m_path;
    if (!m_query.empty()) {
		auto i{ m_query.begin() };
		out += "?" + i->first + "=" + i->second;
		++i;
		for (; i != m_query.end(); ++i) {
			out += "&" + i->first + "=" + i->second;
		}
	}
    out += " HTTP/1.1\r\n";
    out += "Host: " + m_host + "\r\n";
    out += "Content-Length: " + std::to_string(m_body.size()) + "\r\n";
    out += "\r\n" + m_body;
    return out;
}

export class http_response_t {
private:
    std::string m_protocol{};
    std::string m_status_code{};
    std::string m_status_text{};
    std::map<std::string, std::string> m_header{};
    std::string m_body{};
public:
    http_response_t() = default;
    http_response_t(std::string response);
    
    auto protocol_get() const -> std::string;
    auto status_code_get() const -> std::string;
    auto status_text_get() const -> std::string;
    auto header_get(std::string name) const -> std::string;
    auto body_get() const -> std::string;
};

http_response_t::http_response_t(std::string response) {
    std::uint64_t pos_end{ response.find(" ") };
    m_protocol = response.substr(0, pos_end);
    std::uint64_t pos_begin{ pos_end + 1 };
    pos_end = response.find(" ", pos_begin);
    m_status_code = response.substr(pos_begin, pos_end - pos_begin);
    pos_begin = pos_end + 1;
    pos_end = response.find("\r\n", pos_begin);
    m_status_text = response.substr(pos_begin, pos_end - pos_begin);
    pos_begin = pos_end + 2;
    while (true) {
        pos_end = response.find("\r\n", pos_begin);
        if (pos_end == pos_begin) {
            pos_begin = pos_end + 2;
            break;
        }
        std::string header_name{}, header_val{};
        std::uint64_t pos_colon{ response.find(": ", pos_begin) };
        header_name = response.substr(pos_begin, pos_colon - pos_begin);
        pos_colon += 2;
        header_val = response.substr(pos_colon, pos_end - pos_colon);
        m_header[header_name] = header_val;
        pos_begin = pos_end + 2;
    }
    m_body = response.substr(pos_begin);
    return;
}

auto http_response_t::protocol_get() const -> std::string {
    return m_protocol;
}

auto http_response_t::status_code_get() const -> std::string {
    return m_status_code;
}

auto http_response_t::status_text_get() const -> std::string {
    return m_status_text;
}

auto http_response_t::header_get(std::string name) const -> std::string {
    return m_header.at(name);
}

auto http_response_t::body_get() const -> std::string {
    return m_body;
}

export class curl_multipart_t {
private:
    struct http_t {
        http_request_t m_request{};
        http_response_t m_response{};
        bool m_successful{};
    };
    std::string m_path{};
    std::string m_host{};
    std::map<std::string, std::string> m_query{};
    std::map<std::string, std::string> m_header{};
    std::map<std::uint64_t, http_t> m_http{};
    std::string m_boundary{};
    CURL* m_curl{};
    std::uint64_t m_id_current{};
public:
    curl_multipart_t(CURL* curl);
    
    auto size() const -> std::uint64_t;
    auto clear() -> void;
    auto host_get() const -> std::string;
    auto host_set(std::string host) -> void;
    auto path_get() const -> std::string;
    auto path_set(std::string path) -> void;
    auto query_get(std::string key) const -> std::string;
    auto query_set(std::string key, std::string val) -> void;
    auto header_get(std::string name) const -> std::string;
    auto header_set(std::string name, std::string val) -> void;
    auto boundary_get() const -> std::string;
    auto boundary_set(std::string boundary) -> void;
    auto request_add() -> std::uint64_t;
    auto request_get(std::uint64_t id) -> http_request_t&;
    auto response_get(std::uint64_t id) -> http_response_t&;
    auto action_try() -> void;
    auto action() -> void;
};

curl_multipart_t::curl_multipart_t(CURL* curl): m_curl{ curl } {}

auto curl_multipart_t::size() const -> std::uint64_t {
    return m_http.size();
}

auto curl_multipart_t::clear() -> void {
    m_http.clear();
    return;
}

auto curl_multipart_t::host_get() const -> std::string {
    return m_host;
}

auto curl_multipart_t::host_set(std::string host) -> void {
    m_host = host;
    return;
}

auto curl_multipart_t::path_get() const -> std::string {
    return m_path;
}

auto curl_multipart_t::path_set(std::string path) -> void {
    m_path = path;
    return;
}

auto curl_multipart_t::query_get(std::string key) const -> std::string {
    return m_query.at(key);
}

auto curl_multipart_t::query_set(std::string key, std::string val) -> void {
    m_query[key] = val;
    return;
}

auto curl_multipart_t::header_get(std::string name) const -> std::string {
    return m_header.at(name);
}

auto curl_multipart_t::header_set(std::string name, std::string val) -> void {
    m_header[name] = val;
    return;
}

auto curl_multipart_t::boundary_get() const -> std::string {
    return m_boundary;
}

auto curl_multipart_t::boundary_set(std::string boundary) -> void {
    m_boundary = boundary;
    return;
}

auto curl_multipart_t::request_add() -> std::uint64_t {
    m_http[m_id_current] = http_t{};
    return m_id_current++;
}

auto curl_multipart_t::request_get(std::uint64_t id) -> http_request_t& {
    return m_http[id].m_request;
}

auto curl_multipart_t::response_get(std::uint64_t id) -> http_response_t& {
    return m_http[id].m_response;
}

auto curl_multipart_t::action_try() -> void {
    if (!size()) {
        return;
    }
	curl_slist* header_curl{};
    std::string body{};
    body += "--" + m_boundary + "\r\n";
    std::uint64_t count{ 0 };
    for (auto i : m_http) {
        if (i.second.m_successful) {
            continue;
        }
        std::string content{ i.second.m_request.to_str() };
        body += "Content-Type: application/http\r\n";
        body += "Content-Transfer-Encoding: binary\r\n";
        body += "Content-ID: " + std::to_string(i.first) + "\r\n";
        body += "\r\n" + content + "\r\n";
        body += "--" + m_boundary + "\r\n";
        ++count;
    }
    body.pop_back();
    body.pop_back();
    body += "--\r\n";
	std::string response{};
	debug_str_t debug_str{};
    curl_header* content_type{};
    header_curl = curl_slist_append(header_curl, ("Content-Type: multipart/mixed; boundary=" + m_boundary).c_str());
    for (auto i : m_header) {
        header_curl = curl_slist_append(header_curl, (i.first + ": " + i.second).c_str());
    }
    while (true) {
        try {
            api_curl(curl_easy_setopt(m_curl, CURLOPT_URL, (m_host + m_path).c_str()));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, header_curl));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_POST, 1L));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size())));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_COPYPOSTFIELDS, body.data()));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, static_cast<void*>(&response)));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &write_callback));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_DEBUGDATA, static_cast<void*>(&debug_str)));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, &debug_callback));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 30L));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_LOW_SPEED_TIME, 30L));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_LOW_SPEED_LIMIT, 30L));
            api_curl(curl_easy_perform(m_curl));
            api_curl(curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 0L));
            api_curlh(curl_easy_header(m_curl, "Content-Type", 0, CURLH_HEADER, -1, &content_type));
        }
        catch (error_curl_t error) {
            log_file(error.what() + "\n");
            continue;
        }
        curl_slist_free_all(header_curl);
        break;
    }
    std::string boundary{ content_type->value };
    std::string boundary_name{ "; boundary=" };
    boundary = "--" + boundary.substr(boundary.find(boundary_name) + boundary_name.size());
    std::uint64_t pos_begin{ response.find(boundary) + boundary.size() + 2 };
    for (std::uint64_t i{ 0 }; i < count; ++i) {
        std::string content_id_name{ "Content-ID: response-" };
        std::uint64_t content_id_begin{ response.find(content_id_name, pos_begin) + content_id_name.size() };
        std::uint64_t content_id_end{ response.find("\r\n", content_id_begin) };
        std::uint64_t content_id{ std::stoull(response.substr(content_id_begin, content_id_end - content_id_begin)) };
        pos_begin = response.find("\r\n\r\n", pos_begin) + 2;
        std::uint64_t pos_end{ response.find(boundary, pos_begin) };
        std::string content{ response.substr(pos_begin, pos_end - pos_begin) };
        pos_begin = pos_end + boundary.size() + 2;
        http_response_t http_response{ content };
        m_http[content_id].m_response = http_response;
        m_http[content_id].m_successful = http_response.status_code_get().starts_with('2');
    }
    return;
}

auto curl_multipart_t::action() -> void {
    std::chrono::seconds sleep_duration{ 1 };
    while (true) {
        action_try();
        bool successful{ true };
        for (auto i : m_http) {
            successful = successful && i.second.m_successful;
        }
        if (successful) {
            break;
        }
        std::this_thread::sleep_for(sleep_duration);
        sleep_duration *= 2;
    }
    return;
}
