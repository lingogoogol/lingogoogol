module;

#include <nlohmann/json.hpp>

module storage.google.drive.blob;

import std;

import storage.http;
import storage.google.batch;
import storage.google.account;

namespace
{
    auto upload_metadata(const google_easy_t& easy, const std::string& name,
                         const std::string& content) -> nlohmann::json
    {
        auto account = easy.m_account_context ? std::static_pointer_cast<google_account_t>(easy.m_account_context) :
                                               google_account_t::default_shared();
        auto data = nlohmann::json{{"name",name},{"description",content}};
        // Each metadata blob, including the logical root and reserve blobs,
        // must be created in the hidden space when using drive.appdata.
        if (account->appdata()) data["parents"] = {"appDataFolder"};
        return data;
    }
}

auto blob_upload(google_easy_t& easy, std::string name, std::string content) -> void
{
	easy.clear();
	easy.path_set("/drive/v3/files");
	easy.query_set("fields", "id");
	easy.header_set("Content-Type", "application/json; charset=UTF-8");
    easy.method_post
    (
        upload_metadata(easy,name,content).dump()
    );
    return;
}

auto blob_upload(google_batch_t& batch, std::string name, std::string content) -> void
{
    http_request_t& request{ batch.request_add_handle() };
    request.method_set("POST");
    request.path_set("/drive/v3/files");
    request.query_set("fields", "id");
    request.header_set("Content-Type", "application/json; charset=UTF-8");
    request.body_set
    (
        upload_metadata(batch,name,content).dump()
    );
    return;
}

auto blob_upload_run(google_easy_t& easy, std::string name, std::string content) -> std::string
{
	easy.clear();
	easy.path_set("/drive/v3/files");
	easy.query_set("fields", "id");
	easy.header_set("Content-Type", "application/json; charset=UTF-8");
    easy.method_post
    (
        upload_metadata(easy,name,content).dump()
    );
    easy.run();
	std::string response{ easy.payload() };
	return nlohmann::json::parse(response)["id"];
}

auto blob_update(google_easy_t& easy, blob_opt_t blob) -> void
{
	easy.clear();
	easy.path_set("/drive/v3/files/" + blob.m_id);
    std::string field{};
	if (blob.m_name)
	{
		field += "name,";
	}
	if (blob.m_content)
	{
		field += "description,";
	}
	field.pop_back();
	easy.query_set("fields", field);
    nlohmann::json payload{};
    if (blob.m_name)
    {
        payload["name"] = blob.m_name.value().dump();
    }
    if (blob.m_content)
    {
        payload["description"] = blob.m_content.value().dump();
    }
	easy.method_patch(payload.dump());
    return;
}

auto blob_update(google_easy_t& easy, blob_str_opt_t blob) -> void
{
	easy.clear();
	easy.path_set("/drive/v3/files/" + blob.m_id);
    std::string field{};
	if (blob.m_name)
	{
		field += "name,";
	}
	if (blob.m_content)
	{
		field += "description,";
	}
	field.pop_back();
	easy.query_set("fields", field);
    nlohmann::json payload{};
    if (blob.m_name)
    {
        payload["name"] = blob.m_name;
    }
    if (blob.m_content)
    {
        payload["description"] = blob.m_content;
    }
	easy.method_patch(payload.dump());
	return;
}

auto blob_update(google_batch_t& batch, std::string id, std::string payload) -> void
{
    http_request_t& request{ batch.request_add_handle() };
    request.method_set("PATCH");
    request.path_set("/drive/v3/files/" + id);
    request.header_set("Content-Type", "application/json; charset=UTF-8");
    request.body_set(payload);
    return;
}

auto blob_update(google_batch_t& batch, blob_opt_t blob) -> void
{
    nlohmann::json body{};
    if (blob.m_name)
    {
        body["name"] = blob.m_name->dump();
    }
    if (blob.m_content)
    {
        body["description"] = blob.m_content->dump();
    }
    blob_update
    (
        batch,
        blob.m_id,
        body.dump()
    );
    return;
}

auto blob_update(google_batch_t& batch, blob_str_opt_t blob) -> void
{
    nlohmann::json body{};
    if (blob.m_name)
    {
        body["name"] = blob.m_name;
    }
    if (blob.m_content)
    {
        body["description"] = blob.m_content;
    }
    blob_update
    (
        batch,
        blob.m_id,
        body.dump()
    );
    return;
}

auto blob_update_run(google_easy_t& easy, blob_str_opt_t blob) -> void
{
	easy.clear();
	easy.path_set("/drive/v3/files/" + blob.m_id);
	easy.query_set("fields", "description");//...
    nlohmann::json payload{};
    if (blob.m_name)
    {
        payload["name"] = blob.m_name;
    }
    if (blob.m_content)
    {
        payload["description"] = blob.m_content;
    }
    easy.method_patch(payload.dump());
	easy.run();
	return;
}

auto blob_update_name(google_easy_t& easy, std::string id, std::string name) -> void
{
    blob_update
    (
        easy,
        blob_str_opt_t
        {
            .m_id{ id },
            .m_name{ name }
        }
    );
    return;
}

auto blob_update_name_run(google_easy_t& easy, std::string id, std::string name) -> void
{
    blob_update_run
    (
        easy,
        blob_str_opt_t
        {
            .m_id{ id },
            .m_name{ name }
        }
    );
    return;
}

auto blob_update_content(google_easy_t& easy, std::string id, std::string content) -> void
{
    blob_update
    (
        easy,
        blob_str_opt_t
        {
            .m_id{ id },
            .m_content{ content }
        }
    );
    return;
}

auto blob_update_content_run(google_easy_t& easy, std::string id, std::string content) -> void
{
    blob_update_run
    (
        easy,
        blob_str_opt_t
        {
            .m_id{ id },
            .m_content{ content }
        }
    );
    return;
}

auto blob_exist_run(google_easy_t& easy, std::string id) -> bool
{
	easy.clear();
	easy.path_set("/drive/v3/files/" + id);
    easy.method_get();
    easy.run();
	std::string response{ easy.payload() };
	return !nlohmann::json::parse(response).contains("error");
}

auto blob_get(google_easy_t& easy, std::string id, std::string field) -> void
{
	easy.clear();
	easy.path_set("/drive/v3/files/" + id);
	easy.query_set("fields", field);
    easy.method_get();
    return;
}

auto blob_get(google_batch_t& batch, std::string id, std::string field) -> void
{
    http_request_t& request{ batch.request_add_handle() };
    request.method_set("GET");
    request.path_set("/drive/v3/files/" + id);
    request.query_set("fields", field);
    return;
}

auto blob_get_run(google_easy_t& easy, std::string id, std::string field) -> nlohmann::json
{
	easy.clear();
	easy.path_set("/drive/v3/files/" + id);
	easy.query_set("fields", field);
    easy.method_get();
    easy.run();
	std::string response{ easy.payload() };
    return nlohmann::json::parse(response);
}

auto blob_get_run(google_easy_t& easy, blob_boo_t blob) -> blob_data_str_t
{
	std::string field{};
	if (blob.m_name)
	{
		field += "name,";
	}
	if (blob.m_content)
	{
		field += "description,";
	}
	field.pop_back();
	//Use `=` instead of `{}`: `nlohmann::json{ expr }` invokes the `std::initializer_list` constructor
	//and wraps `expr` into a 1-element JSON array instead of copy/move-constructing the JSON value.
	nlohmann::json response = blob_get_run(easy, blob.m_id, field);
	blob_data_str_t out{};
	if (blob.m_name)
	{
		out.m_name = response["name"];
	}
	if (blob.m_content)
	{
		out.m_content = response["description"];
	}
	return out;
}

auto blob_get_name(google_easy_t& easy, std::string id) -> void
{
    blob_get(easy, id, "name");
    return;
}

auto blob_get_name_run(google_easy_t& easy, std::string id) -> std::string
{
    return blob_get_run(easy, id, "name")["name"];
}

auto blob_get_content(google_easy_t& easy, std::string id) -> void
{
    blob_get(easy, id, "description");
    return;
}

auto blob_get_content_run(google_easy_t& easy, std::string id) -> std::string
{
    return blob_get_run(easy, id, "description")["description"];
}
