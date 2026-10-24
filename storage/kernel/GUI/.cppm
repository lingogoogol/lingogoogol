export module storage.kernel.GUI;

import std;

import lgo.GUI;

import storage.http;
import storage.google;
import storage.kernel.load;

export
{
    struct GUI_persistent_t
    {
        std::shared_ptr<lgo::text_line_std_t> m_frame_rate{};
        std::shared_ptr<lgo::text_line_std_t> m_blob_queue_cycle_rate{};
        std::shared_ptr<lgo::text_line_std_t> m_blob_queue_count{};
        std::vector<std::shared_ptr<lgo::text_line_std_t>> m_load_easy_status{};
    };

    struct easy_status_t
    {
        std::size_t m_text_index{};
        std::chrono::steady_clock::time_point m_add_time{};
    };

    auto message_loop
    (
        lgo::engine_t* engine,
        GUI_persistent_t* persistent,
        curl_multi_t* multi,
	    load_state_t* load_state,
	    blob_queue_t* blob_queue
    )
    -> bool;
    
    auto dialog_file(bool folder) -> std::vector<std::string>;

    auto folder_create_callback(lgo::engine_t* engine, lgo::state_t* state, std::string* pos) -> void;

    auto file_upload_callback
    (
        load_state_t* load_state,
        blob_queue_t* blob_queue,
        std::string pos
    )
    -> void;
    
    auto folder_upload_callback
    (
        load_state_t* load_state,
        blob_queue_t* blob_queue,
        std::string pos
    )
    -> void;

    auto file_download_callback
    (
        load_state_t* load_state,
        std::string name,
        std::string pos
    )
    -> void;

    auto folder_download_callback
    (
        load_state_t* load_state,
        std::string name,
        std::string pos
    )
    -> void;
    
    auto delete_all_callback
    (
        std::shared_ptr<lgo::divy_hard_t> center_div,
        std::shared_ptr<lgo::divy_soft_t> progress_bar_div,
	    load_state_t* load_state
    )
    -> void;

    auto delete_all_confirm_callback
    (
        std::shared_ptr<lgo::divy_hard_t> center_div,
        std::shared_ptr<lgo::divy_soft_t> progress_bar_div,
        std::shared_ptr<lgo::text_line_flex_std_input_t> input,
	    load_state_t* load_state
    )
    -> void;

    auto delete_all_cancel_callback(std::shared_ptr<lgo::divy_hard_t> center_div) -> void;

    auto folder_button_callback
    (
        lgo::engine_t* engine,
        GUI_persistent_t* persistent,
        curl_multi_t* multi,
        load_state_t* load_state,
        blob_queue_t* blob_queue,
        std::shared_ptr<lgo::divy_soft_t> viewer_div,
        std::string* pos_ptr,
        std::string pos
    )
    -> void;

    auto list
    (
        lgo::engine_t* engine,
        GUI_persistent_t* persistent,
        curl_multi_t* multi,
        load_state_t* load_state,
        blob_queue_t* blob_queue,
        std::shared_ptr<lgo::divy_soft_t> viewer_div,
        std::string* pos
    )
    -> void;

    auto home
    (
        lgo::engine_t* engine,
        lgo::state_t* state,
        GUI_persistent_t* persistent,
        std::shared_ptr<lgo::divy_soft_t>* progress_column,
        load_state_t* load_state,
        curl_multi_t* multi,
        blob_queue_t* blob_queue,
        std::string* pos
    )
    -> void;

    constexpr std::size_t load_easy_count{ 64 };
}
