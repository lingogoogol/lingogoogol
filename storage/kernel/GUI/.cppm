module;
#include <nlohmann/json.hpp>
export module storage.kernel.GUI;

import std;

import external.curl;
import lgo.GUI;
import lgo.math.vec;

import storage.http;
import storage.google;
import storage.kernel.load;

export {
    // Divs position children but do not transform their depth ranges. Keep
    // ordinary content behind diagnostics (.16-.19) and modal dialogs (<.14).
    constexpr lgo::depth_range_t storage_content_depth{.2f, .7f};
    struct easy_status_t
    {
        std::size_t m_text_index{};
        std::chrono::steady_clock::time_point m_add_time{};
    };

    struct GUI_persistent_t
    {
        std::shared_ptr<lgo::text_line_std_t> m_directory_status;
        std::shared_ptr<lgo::scroll_t> m_breadcrumbs;
        std::string m_breadcrumb_folder;
        std::map<std::string, std::uint64_t> m_directory_rows;
        std::shared_ptr<lgo::button_classic_std_t> m_paste_button;
        std::function<void()> m_refresh;
        std::function<void()> m_reset_view;
        std::function<void(storage_settings_t)> m_apply_settings;
        std::function<void()> m_clear_account;
        std::unique_ptr<lgo::modal_dialog_t> m_dialog;
        std::function<bool(bool, const std::vector<std::string>&)> m_dialog_done;
        std::deque<std::function<void()>> m_dialog_queue;
        std::deque<std::function<void()>> m_deferred;
        struct clipboard_t
        {
            std::shared_ptr<storage_session_t> session;
            std::string source;
            nlohmann::json entity;
            bool copy{};
        };
        std::optional<clipboard_t> m_clipboard;
        struct listing_t
        {
            std::uint64_t generation{};
            std::shared_ptr<storage_session_t> session;
            std::shared_ptr<request_control_t> control;
            std::future<nlohmann::json> future;
            std::function<void(nlohmann::json)> apply;
        };
        std::uint64_t m_listing_generation{};
        std::list<listing_t> m_listings;
        bool m_switching{};
        ~GUI_persistent_t();
        std::shared_ptr<lgo::text_line_std_t> m_frame_rate{};
        std::shared_ptr<lgo::text_line_std_t> m_blob_queue_cycle_rate{};
        std::shared_ptr<lgo::text_line_std_t> m_blob_queue_count{};
        std::vector<std::shared_ptr<lgo::text_line_scroll_std_t>> m_load_easy_status{};
        std::shared_ptr<lgo::rect_t> m_diagnostics_backdrop;
        std::shared_ptr<lgo::text_line_scroll_std_t> m_diagnostics_summary;
        std::shared_ptr<lgo::button_classic_std_t> m_diagnostics_previous, m_diagnostics_next;
        std::shared_ptr<lgo::text_line_std_t> m_diagnostics_page_text;
        std::size_t m_diagnostics_page{}, m_diagnostics_page_size{64};
        bool m_diagnostics_visible{};
        auto layout_diagnostics(lgo::pos_2D pos, lgo::size_2D size) -> void;
        auto show_diagnostics(bool show) -> void;
        // Handle ownership and status belong to this GUI/worker pair, never static storage.
        std::vector<google_easy_t> m_easy_idle{};
        std::map<CURL*, easy_status_t> m_easy_status{};
    };

    auto message_loop(lgo::engine_t* engine, GUI_persistent_t* persistent, curl_multi_worker_t* multi,
                      load_state_t* load_state, blob_queue_t* blob_queue) -> bool;

    auto dialog_file(bool folder) -> std::vector<std::string>;

    auto folder_create_callback(lgo::engine_t* engine, GUI_persistent_t* persistent, load_state_t* state, std::string* pos) -> void;

    auto file_upload_callback(load_state_t* load_state, blob_queue_t* blob_queue, std::string pos) -> void;

    auto folder_upload_callback(load_state_t* load_state, blob_queue_t* blob_queue, std::string pos) -> void;

    auto file_download_callback(load_state_t* load_state, std::string name, std::string pos) -> void;

    auto folder_download_callback(load_state_t* load_state, std::string name, std::string pos) -> void;

    auto delete_all_callback(std::shared_ptr<lgo::divy_hard_t> center_div,
                             std::shared_ptr<lgo::divy_soft_t> progress_bar_div, load_state_t* load_state)
        -> void;

    auto delete_all_confirm_callback(std::shared_ptr<lgo::divy_hard_t> center_div,
                                     std::shared_ptr<lgo::divy_soft_t> progress_bar_div,
                                     std::shared_ptr<lgo::text_line_flex_std_input_t> input,
                                     load_state_t* load_state) -> void;

    auto delete_all_cancel_callback(std::shared_ptr<lgo::divy_hard_t> center_div) -> void;

    auto folder_button_callback(lgo::engine_t* engine, GUI_persistent_t* persistent,
                                curl_multi_worker_t* multi, load_state_t* load_state,
                                blob_queue_t* blob_queue, std::shared_ptr<lgo::scroll_t> viewer_div,
                                std::string* pos_ptr, std::string pos) -> void;

    auto list(lgo::engine_t* engine, GUI_persistent_t* persistent, curl_multi_worker_t* multi,
              load_state_t* load_state, blob_queue_t* blob_queue,
              std::shared_ptr<lgo::scroll_t> viewer_div, std::string* pos) -> void;

    auto home(lgo::engine_t* engine, lgo::state_t* state, GUI_persistent_t* persistent,
              std::shared_ptr<lgo::divy_soft_t>* progress_column, load_state_t* load_state,
              curl_multi_worker_t* multi, blob_queue_t* blob_queue, std::string* pos) -> void;

    auto
    restore_tasks(load_state_t* state,
                  const std::function<std::shared_ptr<storage_session_t>(const nlohmann::json&)>& session)
        -> void;
    auto begin_directory_task(load_state_t* state, std::shared_ptr<storage_session_t> session,
                              std::string kind, std::string source, nlohmann::json entity = {},
                              std::string destination = {}, std::string name = {},
                              std::shared_ptr<task_journal_t> journal = {}) -> void;
    auto begin_task(load_state_t* state, std::shared_ptr<storage_session_t> session, std::string kind,
                    std::string path, std::string pos, std::shared_ptr<task_journal_t> journal = {}) -> void;

    constexpr std::size_t load_easy_count{transfer_slot_count};
}
