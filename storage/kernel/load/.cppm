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
    constexpr std::size_t transfer_slot_count{64};
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
    struct directory_task_t;

    struct storage_session_t {
        std::shared_ptr<google_account_t> account;
        std::shared_ptr<blob_queue_t> queue;
        std::string account_path;
        std::string address;
        std::string root_path;
        std::string root;
        bool clearing{};
    };

    struct load_state_t
    {
        std::shared_ptr<storage_session_t> m_session;
        storage_settings_t m_settings;
        std::function<void(std::string, std::function<void(bool)>)> m_confirm;
        std::function<void()> m_settings_page;
        std::function<void()> m_clear_page;
        std::list<std::shared_ptr<load_task_t>> m_task_list{};
        std::weak_ptr<load_task_t> m_next_transfer;
        // Terminal rows stay visible until acknowledged. Owners also outlive
        // cancelled requests and asynchronous preparation/publication.
        std::list<std::shared_ptr<load_task_t>> m_finished_tasks{};
        std::map<CURL*, std::function<void(std::unique_ptr<curl_easy_t>&)>> m_end{};
        struct parked_request_t
        {
            std::unique_ptr<curl_easy_t> request;
            std::function<void(std::unique_ptr<curl_easy_t>&)> complete;
        };
        // Logical requests and callbacks remain; no curl handles or pool slots.
        std::deque<parked_request_t> m_parked;
        std::shared_ptr<lgo::divy_soft_t> m_progress_column{};
        //Serialize read/modify/write publication to existing folders to avoid lost entries.
        std::shared_future<void> m_upload_linked{};
        //Invoked on the GUI thread after publication succeeds.
        std::function<void(const std::string&)> m_on_upload_linked{};
        std::function<void(const directory_task_t&)> m_on_directory_changed;
    };
    
    enum class task_result_t { active, cancelling, completed, cancelled, failed };

    using progress_button_t = lgo::button_text_t<lgo::text_line_std_t>;

    struct load_task_t
    {
        virtual ~load_task_t() = default;

        std::shared_ptr<storage_session_t> m_session;
        std::shared_ptr<request_control_t> m_control{ std::make_shared<request_control_t>() };
        std::shared_ptr<task_journal_t> m_journal;
        std::string m_label;
        // GUI-thread snapshot: pause must retain the current progress/phase,
        // rather than replacing it with the original preparation label.
        std::string m_paused_text;
        std::uint64_t m_saved_progress{};
        task_result_t m_result{task_result_t::active};
        bool m_dismissed{};
        bool m_row_removed{};
        bool m_preparation_failed{};
        std::function<void()> m_retry_prepare;
        auto pause(bool value) -> void
        {
            const bool changed{value != m_control->user_paused.load()};
            if (value && changed && m_progress_text)
                m_paused_text = m_progress_text->get_text();
            // Persist first: restarting the application must not restart a task
            // that the user explicitly paused.
            if (m_journal)
            {
                if (value && changed)
                    m_journal->set("paused_text", m_paused_text);
                m_journal->set("user_paused", value);
            }
            m_control->pause(value);
            if (changed && m_progress_text && !m_control->paused && !m_control->cancelled)
                m_progress_text->set_text(value ? "已暫停：" + m_paused_text : m_paused_text, false);
        }
        std::shared_ptr<lgo::progress_bar_t> m_progress_bar;
        std::shared_ptr<progress_button_t> m_confirm_button;
        std::shared_ptr<progress_button_t> m_pause_button;
        std::shared_ptr<progress_button_t> m_resume_button;
        std::shared_ptr<progress_button_t> m_retry_button;
        std::shared_ptr<progress_button_t> m_cancel_button;
        std::shared_ptr<lgo::std_liney_t> m_action_separator;
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
        //Also required for empty files, whose chunk counters are always zero.
        bool m_load_end{ false };
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
        //An empty folder still needs a successful metadata response.
        bool m_load_end{ false };
    };

    //`upload_task_t` has `upload_folder_t` as a member, so it must be defined after `upload_folder_t`.
    struct upload_task_t: public load_task_t
    {
        ~upload_task_t() override
        {
            // Preparation captures this task and writes its containers. Join before member destruction.
            if (m_prepared.valid()) m_prepared.wait();
        }
        std::string m_name{};
        std::future<void> m_prepared{};
        std::uint64_t m_size{ 0 };
        //The total size in bytes of chunks that has finished transferring to the cloud.
        std::uint64_t m_progress{ 0 };
        upload_folder_t m_pos{};
        //Valid only after every entity has finished uploading successfully.
        std::shared_future<void> m_linked{};
        //The root entity is at `m_entity.begin()`.
        std::vector<std::shared_ptr<upload_entity_t>> m_entity_arr{};
        //The first entity that need to send more requests.
        //Every entities preceding it finished beginning requests.
        std::vector<std::shared_ptr<upload_entity_t>>::iterator m_begin_progress{};
        std::vector<std::shared_ptr<upload_entity_t>>::iterator m_end_progress{};
    };

    struct directory_request_t
    {
        std::function<void(google_easy_t&)> configure;
        std::promise<std::string> response;
        bool allow_missing{};
    };

    struct directory_task_t: public load_task_t
    {
        ~directory_task_t() override
        {
            // A coordinator can be waiting for a GUI-dispatched request during
            // shutdown. Cancel before joining, when no more frames will run.
            m_control->cancel();
            if (m_operation.valid()) m_operation.wait();
        }
        std::string m_kind, m_name, m_source, m_destination;
        nlohmann::json m_entity;
        std::shared_future<void> m_operation;
        std::atomic_uint64_t m_completed{}, m_total{};
        // The coordinator publishes successful unlinking; only the GUI thread
        // consumes the notification and changes visible controls.
        std::atomic_bool m_source_unlinked{};
        bool m_unlink_notified{};
        // Only descriptions cross threads; physical handles come exclusively
        // from the same pool used by uploads and downloads on the GUI thread.
        std::mutex m_requests_mutex;
        std::deque<std::shared_ptr<directory_request_t>> m_requests;
    };

    auto directory_validate_name(std::string_view name) -> void;
    auto directory_execute(directory_task_t& task) -> void;
    auto directory_notify_unlinked(directory_task_t& task) -> void;
    auto directory_transfer_begin(directory_task_t& task, curl_multi_worker_t& multi,
                                  curl_easy_t&& easy) -> bool;

    struct download_chunk_t
    {
        //The download file that this chunk belongs to.
        download_file_t* m_file{};
        std::string m_id{};
        std::vector<std::string> m_content_id{};
        std::vector<std::string> m_content{};
        bool m_loaded{ false };
    };

    struct download_file_t
    {
        //The task that this loading file belongs to.
        download_task_t* m_task{};
        std::string m_name{};
        std::string m_path_local{};
        std::uint64_t m_size{};
        std::uint64_t m_written{};
        std::uint64_t m_resume_written{};
        //`m_idle` means this file can send more requests for now.
        //If `m_idle` is false, the file must wait for a response from the server before it can send more requests.
        //Specifically, m_idle does not mean that the file is not running tranfers.
        //There can be more transfers to run (`m_idle == true`) while already running tranfers.
        bool m_idle{ true };
        //`m_file_loading` is true if it is loading more file blobs into `m_chunk_arr`.
        bool m_file_loading{ false };
        bool m_complete{ false };
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
        ~download_task_t() override
        {
            if (m_prepared.valid()) m_prepared.wait();
        }
        std::string m_name{};
        std::string m_pos{};
        // Failed tasks remain visible; outstanding responses are drained without writes.
        std::string m_error{};
        std::future<void> m_prepared{};
        std::uint64_t m_size{ 0 };
        //The total size in bytes of chunks that has finished writing to local filesystem.
        std::uint64_t m_progress{ 0 };
        std::vector<download_file_t> m_file_arr{};
        std::size_t m_files_remaining{};
        //The first file that can send more requests.
        std::vector<download_file_t>::iterator m_idle_first{};
    };

    auto finish_task(load_task_t& task, task_result_t result, std::string text) -> void;
    auto collect_finished_tasks(load_state_t& state, curl_multi_worker_t& multi) -> void;

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
    auto transfer_begin(load_state_t* state, curl_multi_worker_t& multi, curl_easy_t&& easy, blob_queue_t* blob_queue) -> bool;
    auto transfer_end(load_state_t* state, std::unique_ptr<curl_easy_t>& easy) -> void;

    auto upload_complete(upload_entity_t* entity) -> void;
    auto upload_file_load_begin
    (
        curl_multi_worker_t& multi,
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
    auto upload_chunk_load_begin(curl_multi_worker_t& multi, curl_easy_t&& easy, upload_file_t* file) -> void;
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
        curl_multi_worker_t& multi,
        curl_easy_t&& easy,
        blob_queue_t* blob_queue,
        upload_file_t* file
    )
    -> void;
    auto upload_chunk_transfer_end(std::unique_ptr<curl_easy_t>&, upload_file_t* file, std::size_t chunk_i) -> void;
    auto upload_folder_load_begin
    (
        curl_multi_worker_t& multi,
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
        curl_multi_worker_t& multi,
        curl_easy_t&& easy,
        std::vector<download_file_t>::iterator file_i
    )
    -> void;
    auto download_file_load_end(std::unique_ptr<curl_easy_t>& easy, std::vector<download_file_t>::iterator file_i) -> void;
    auto download_chunk_load_begin
    (
        curl_multi_worker_t& multi,
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
    auto download_chunk_transfer_begin(curl_multi_worker_t& multi, curl_easy_t&& easy, download_chunk_t& chunk) -> void;
    auto download_chunk_transfer_end(std::unique_ptr<curl_easy_t>& easy, download_file_t* file, std::size_t chunk_i) -> void;

    constexpr std::uint64_t content_size_max{ 25000 };
    constexpr std::uint64_t content_size_max_base64{ 18750 };
    std::string folder_root_name{ "folder_root" };
}
