module;

#include <nlohmann/json.hpp>

module storage.google.drive.blob;

import std;

import storage.http;
import storage.google.batch;

auto blob_upload(google_batch_t& batch, std::string name, std::string content) -> void
{
    http_request_t& request{ batch.request_add_handle() };
    request.method_set("POST");
    request.path_set("/drive/v3/files");
    request.query_set("fields", "id");
    request.header_set("Content-Type", "application/json; charset=UTF-8");
    request.body_set
    (
        nlohmann::json
        {
            { "name", name },
            { "description", content }
        }
        .dump()
    );
    return;
}

auto blob_update(google_batch_t& batch, std::string id, std::string body) -> void
{
    http_request_t& request{ batch.request_add_handle() };
    request.method_set("PATCH");
    request.path_set("/drive/v3/files/" + id);
    request.body_set(body);
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

auto blob_download(google_batch_t& batch, std::string id) -> void
{
    http_request_t& request{ batch.request_add_handle() };
    request.method_set("GET");
    request.path_set("/drive/v3/files/" + id);
    request.query_set("fields", "description");
    return;
}
