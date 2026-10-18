module;

#include <nlohmann/json.hpp>

export module storage.kernel.load;

import std;

import external.curl;

import lgo.GUI;

import storage.http;
import storage.google;

//Structure of a file blob's name:
//{
//  "name": the name of the file (str),
//  "size": the size of the file (int),
//  "parent": the directory id this file belongs to (str),
//  "next": the id of next file blob of the same file (str)
//}

//Structure of a file blob's description:
//[
//  up to 100 ids of the file's chunk blobs (str)
//]

//Structure of a file chunk blob's name:
//{
//  "parent": the file blob id this chunk belongs to (str)
//}

//Structure of a chunk blob's description:
//[
//  up to 100 ids of the chunk's file content blobs (str)
//]

//Structure of a file content blob's name:
//{
//  "parent": the chunk blob id this content belongs to (str)
//}

//A file content blob's description contains base64 encoding of some file content.

//Structure of a folder blob's name:
//{
//  "name": the name of the folder (str),
//  "parent": the directory id this file belongs to (str),
//  "next": the id of next folder blob of the same file (str)
//}

//Structure of a folder blob's description:
//[
//  up to 100
//  {
//      "id": the id of the entity blob (str),
//      "type": "file" or "folder" (str),
//      "name": the name of the entity (str),
//      "size": size of the file. This field is only present in the links to files, not folders. (int)
//  }
//]

export
{
    struct load_state_t;
    struct load_task_t;
    struct upload_task_t;
    struct upload_entity_t;
    struct upload_file_t;
    struct upload_chunk_t;
    struct upload_folder_t;
    struct download_task_t;
    struct download_file_t;
    struct download_chunk_t;

    struct load_state_t
    {
        std::list<std::shared_ptr<load_task_t>> m_task_list{};
        std::map<CURL*, std::function<void(std::unique_ptr<curl_easy_t>&)>> m_end{};
        std::shared_ptr<lgo::divy_soft_t> m_progress_column{};
    };
    
    struct load_task_t
    {
        virtual ~load_task_t() = default;

        load_state_t* m_state{};
        std::list<std::shared_ptr<load_task_t>>::iterator m_self_i{};
        std::string m_type{};
        std::uint64_t m_progress_div{};
        std::shared_ptr<lgo::text_line_scroll_std_t> m_progress_text{};
    };
    
    struct upload_entity_t
    {
        virtual ~upload_entity_t() = default;

        upload_task_t* m_task{};
        upload_folder_t* m_parent{};
        std::string m_type{};
        std::string m_name{};
        std::size_t m_self_i{};
    };

    struct upload_chunk_t
    {
        //The upload file that this chunk belongs to.
        upload_file_t* m_file{};
        std::string m_id{};
        std::vector<std::string> m_content_id{};
        std::uint64_t m_size{};
        bool m_transfer_end{ false };
        bool m_load_end{ false };
        bool m_link_end{ false };
    };

    struct upload_file_t: public upload_entity_t
    {
        std::string m_main_id{};
        std::string m_next_id{};
        std::string m_path_local{};
        //The total size in bytes of the file.
        std::uint64_t m_size{};
        //The total size in bytes that is covered by chunks that began loading.
        std::uint64_t m_chunk_cover_progress{ 0 };
        //The total number of chunks that began linking to a file blob.
        std::size_t m_file_load_begin_progress{ 0 };
        //The first chunk that has not finished linking to a file blob.
        std::size_t m_file_load_end_progress{ 0 };
        //The total number of chunk blobs that began creating in the cloud.
        std::size_t m_chunk_load_begin_progress{ 0 };
        //The first chunk blob that has not finished creating in the cloud.
        std::size_t m_chunk_load_end_progress{ 0 };
        //The first chunk that has not finished transfer.
        std::size_t m_chunk_transfer_end_progress{ 0 };
        std::vector<upload_chunk_t> m_chunk_arr{};
    };

    struct upload_folder_t: public upload_entity_t
    {
        std::string m_main_id{};
        std::string m_next_id{};
        //The total number of entities that began linking to a folder blob.
        std::uint64_t m_folder_load_begin_progress{ 0 };
        //The first chunk that has not finished linking to a folder blob.
        std::uint64_t m_folder_load_end_progress{ 0 };
        std::vector<std::size_t> m_entity_arr{};
        std::vector<bool> m_entity_load_end{};
    };

    //`upload_task_t` has `upload_folder_t` as a member, so it must be defined after `upload_folder_t`.
    struct upload_task_t: public load_task_t
    {
        std::string m_name{};
        std::future<void> m_prepared{};
        std::uint64_t m_size{ 0 };
        //The total size in bytes of chunks that has finished transferring to the cloud.
        std::uint64_t m_progress{ 0 };
        upload_folder_t m_pos{};
        //The root entity is at `m_entity.begin()`.
        std::vector<std::shared_ptr<upload_entity_t>> m_entity_arr{};
        //The first entity that need to send more requests.
        //Every entities preceding it finished beginning requests.
        std::vector<std::shared_ptr<upload_entity_t>>::iterator m_begin_progress{};
        std::vector<std::shared_ptr<upload_entity_t>>::iterator m_end_progress{};
    };

    struct download_chunk_t
    {
        //The download file that this chunk belongs to.
        download_file_t* m_file{};
        std::string m_id{};
        std::vector<std::string> m_content_id{};
        std::vector<std::string> m_content{};
    };

    struct download_file_t
    {
        //The task that this loading file belongs to.
        download_task_t* m_task{};
        std::string m_name{};
        std::string m_path_local{};
        std::uint64_t m_size{};
        //`m_idle` means this file can send more requests for now.
        //If `m_idle` is false, the file must wait for a response from the server before it can send more requests.
        //Specifically, m_idle does not mean that the file is not running tranfers.
        //There can be more transfers to run (`m_idle == true`) while already running tranfers.
        bool m_idle{ true };
        //`m_file_loading` is true if it is loading more file blobs into `m_chunk_arr`.
        bool m_file_loading{ false };
        //The first chunk that needs to be loaded.
        //We can't use an iterator for this variable because this file may still be loading,
        // and `m_chunk_arr` may be modified, invalidating the iterator.
        std::uint64_t m_chunk_load_begin_progress{ 0 };
        //The first chunk that the loading process hasn't ended.
        //We can't use an iterator for this variable because this file may still be loading,
        // and `m_chunk_arr` may be modified, invalidating the iterator.
        std::uint64_t m_chunk_load_end_progress{ 0 };
        //The first chunk that needs to be transferred.
        //We can't use an iterator for this variable because this file may still be loading,
        // and `m_chunk_arr` may be modified, invalidating the iterator.
        std::uint64_t m_chunk_transfer_begin_progress{ 0 };
        //The first chunk that has not finished writing to local filesystem.
        //We can't use an iterator for this variable because this file may still be loading,
        // and `m_chunk_arr` may be modified, invalidating the iterator.
        std::uint64_t m_filesystem_progress{ 0 };
        std::vector<download_chunk_t> m_chunk_arr{};
        //`m_next_id` equal to `"null"` indicates that chunk loading has completed.
        std::string m_next_id{};
    };

    struct download_task_t: public load_task_t
    {
        std::string m_name{};
        std::string m_pos{};
        std::future<void> m_prepared{};
        std::uint64_t m_size{ 0 };
        //The total size in bytes of chunks that has finished writing to local filesystem.
        std::uint64_t m_progress{ 0 };
        std::vector<download_file_t> m_file_arr{};
        //The first file that can send more requests.
        std::vector<download_file_t>::iterator m_idle_first{};
    };

    auto list_folder_content(google_easy_t& easy, std::string* pos) -> nlohmann::json;

    auto file_upload
    (
        blob_queue_t* blob_queue,
        upload_task_t* task,
        std::filesystem::path path_local,
        upload_folder_t* parent
    )
    -> std::size_t;
    auto folder_upload
    (
        blob_queue_t* blob_queue,
        upload_task_t* task,
        std::filesystem::path path_local,
        upload_folder_t* parent
    )
    -> std::size_t;

    auto file_download
    (
        google_easy_t* easy,
        download_task_t* task,
        std::string pos,
        std::filesystem::path path_local
    )
    -> void;
    auto folder_download
    (
        google_easy_t* easy,
        download_task_t* task,
        std::string pos,
        std::filesystem::path path_local
    )
    -> void;

    auto folder_data_add
    (
        google_easy_t& easy,
        std::string last_id,
        std::string name,
        std::string parent,
        nlohmann::json content = nlohmann::json::array()
    )
    -> std::string;
    auto folder_add
    (
        google_easy_t& easy,
        std::string pos,
        nlohmann::json entity
    )
    -> void;

    auto folder_root_create(google_easy_t& easy) -> std::string;
    auto transfer_begin(load_state_t* state, curl_multi_t& multi, curl_easy_t&& easy, blob_queue_t* blob_queue) -> bool;
    auto transfer_end(load_state_t* state, std::unique_ptr<curl_easy_t>& easy) -> void;

    auto upload_complete(upload_entity_t* entity) -> void;
    auto upload_file_load_begin
    (
        curl_multi_t& multi,
        curl_easy_t&& easy,
        blob_queue_t* blob_queue,
        upload_file_t* file
    )
    -> void;
    auto upload_file_load_end
    (
        std::unique_ptr<curl_easy_t>&,
        upload_file_t* file,
        std::uint64_t begin_i,
        std::uint64_t chunk_count
    )
    -> void;
    auto upload_chunk_load_begin(curl_multi_t& multi, curl_easy_t&& easy, upload_file_t* file) -> void;
    auto upload_chunk_load_end
    (
        std::unique_ptr<curl_easy_t>&,
        upload_file_t* file,
        std::uint64_t begin_i,
        std::uint64_t chunk_count
    )
    -> void;
    auto upload_chunk_transfer_begin
    (
        curl_multi_t& multi,
        curl_easy_t&& easy,
        blob_queue_t* blob_queue,
        upload_file_t* file
    )
    -> void;
    auto upload_chunk_transfer_end(std::unique_ptr<curl_easy_t>&, upload_file_t* file, std::size_t chunk_i) -> void;
    auto upload_folder_load_begin
    (
        curl_multi_t& multi,
        curl_easy_t&& easy,
        blob_queue_t* blob_queue,
        upload_folder_t* folder
    )
    -> void;
    auto upload_folder_load_end
    (
        std::unique_ptr<curl_easy_t>&,
        upload_folder_t* folder,
        std::uint64_t begin_i,
        std::uint64_t entity_count
    )
    -> void;

    auto download_idle_update(std::vector<download_file_t>::iterator file_i) -> void;
    auto download_file_load_begin
    (
        curl_multi_t& multi,
        curl_easy_t&& easy,
        std::vector<download_file_t>::iterator file_i
    )
    -> void;
    auto download_file_load_end(std::unique_ptr<curl_easy_t>& easy, std::vector<download_file_t>::iterator file_i) -> void;
    auto download_chunk_load_begin
    (
        curl_multi_t& multi,
        curl_easy_t&& easy,
        std::vector<download_file_t>::iterator file_i
    )
    -> void;
    auto download_chunk_load_end
    (
        std::unique_ptr<curl_easy_t>& easy,
        std::vector<download_file_t>::iterator file_i,
        std::size_t i_begin
    )
    -> void;
    auto download_chunk_transfer_begin(curl_multi_t& multi, curl_easy_t&& easy, download_chunk_t& chunk) -> void;
    auto download_chunk_transfer_end(std::unique_ptr<curl_easy_t>& easy, download_chunk_t* chunk) -> void;

    constexpr std::uint64_t content_size_max{ 25000 };
    constexpr std::uint64_t content_size_max_base64{ 18750 };
    std::string folder_root_name{ "folder_root" };
}
