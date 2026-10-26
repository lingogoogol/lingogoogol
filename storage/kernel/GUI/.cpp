module;

#include <nlohmann/json.hpp>

module storage.kernel.GUI;

import std;

import external.curl;
import external.GLFW;

import lgo.dev;
import lgo.math;
import lgo.GUI;

import storage.dialog;
import storage.http;
import storage.google;
import storage.kernel.load;

auto message_loop
(
	lgo::engine_t* engine,
	GUI_persistent_t* persistent,
	curl_multi_worker_t* multi,
	load_state_t* load_state,
	blob_queue_t* blob_queue
)
-> bool
{
	using namespace std::chrono_literals;
    blob_queue->check_error();
	
    auto& easy_idle_arr{ persistent->m_easy_idle };
    auto& load_easy_status_map{ persistent->m_easy_status };
    if (load_easy_status_map.empty())
    {
        easy_idle_arr.resize(load_easy_count);
        for (std::size_t i{}; i < easy_idle_arr.size(); ++i)
            load_easy_status_map[easy_idle_arr[i].interface_get()].m_text_index = i;
    }

	persistent->m_blob_queue_cycle_rate->set_text("blob_queue_cycle_rate: " + std::to_string(blob_queue->cycle_rate()));
	persistent->m_blob_queue_count->set_text("blob_queue_count: " + std::to_string(blob_queue->count()));
	if (glfwGetKey(engine->window_get(), GLFW_KEY_F3))
	{
		for (auto& load_easy_status : load_easy_status_map)
		{
			persistent->m_load_easy_status[load_easy_status.second.m_text_index]->show();
		}
	}
	else
	{
		for (auto& load_easy_status : load_easy_status_map)
		{
			persistent->m_load_easy_status[load_easy_status.second.m_text_index]->hide();
		}
	}
	for (auto& load_easy_status : load_easy_status_map)
	{
		std::string status_str{};
		if (load_easy_status.second.m_add_time.time_since_epoch() == std::chrono::steady_clock::duration::zero())
		{
			status_str = "idle";
		}
		else
		{
			status_str = std::format
				(
					"{:%T}, {}",
					std::chrono::steady_clock::now() - load_easy_status.second.m_add_time,
					multi->read_progress(load_easy_status.first)
				);
		}
		persistent->m_load_easy_status[load_easy_status.second.m_text_index]->set_text
		(
			std::format("{:#018x}, {}", reinterpret_cast<std::uint64_t>(load_easy_status.first), status_str)
		);
	}

    // Submit at most one new request per frame; only the worker performs curl.
    if (!easy_idle_arr.empty())
    {
		CURL* key{ easy_idle_arr.back().interface_get() };
		if (transfer_begin(load_state, *multi, std::move(easy_idle_arr.back()), blob_queue))
		{
			load_easy_status_map[key].m_add_time = std::chrono::steady_clock::now();
			easy_idle_arr.pop_back();
		}
	}
	multi->run();
    // Bound completion work so a burst of finished requests still permits input/rendering.
	for (std::size_t completed{}; completed < 8; ++completed)
	{
		if (!multi->ready_nonempty())
		{
			break;
		}
		std::unique_ptr<curl_easy_t> easy{ multi->ready_get_unique() };
		transfer_end(load_state, easy);
		load_easy_status_map[easy->interface_get()].m_add_time = {};
		easy_idle_arr.push_back(std::move(dynamic_cast<google_easy_t&>(*easy.get())));
	}
	return engine->message_loop();
}

auto dialog_file(bool folder) -> std::vector<std::string>
{
	// IFileDialog::Show is modal: the constructor returns after selection or cancellation.
	// Waiting for ok() after cancellation would loop forever and ignore window-close requests.
	dialog_t dialog{ folder };
	return dialog.result();
}

auto folder_create_callback(lgo::engine_t* engine, lgo::state_t* state, std::string* pos) -> void
{
	return;
}

auto file_upload_callback
(
	load_state_t* load_state,
	blob_queue_t* blob_queue,
	std::string pos
)
-> void
{
	std::vector<std::string> file_arr
	{
		dialog_file(false)
	};
	for (auto file : file_arr)
	{
		auto task{ std::make_shared<upload_task_t>() };
		task->m_state = load_state;
		task->m_type = "upload";
		std::uint64_t progress_div_id
		{
			load_state->m_progress_column->add_object<lgo::divy_flex_soft_t>
			(
				lgo::depth_range_t{ 0.0f, 0.5f },
				load_state->m_progress_column->space_x(lgo::std_margin),
				lgo::std_margin,
				lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::bottom },
				true
			)
		};
		task->m_progress_div = progress_div_id;
		load_state->m_progress_column->add_object<lgo::std_linex_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			load_state->m_progress_column->space_x(lgo::std_margin),
			lgo::std_white
		);
		task->m_progress_text =
			load_state->m_progress_column->get_object_shared<lgo::divy_flex_soft_t>(progress_div_id)->
				add_object_shared<lgo::text_line_scroll_std_t>
			(
				lgo::depth_range_t{ 0.0f, 0.0f },
				"檔案：0B/?B " + file,
				lgo::std_blue_light,
				lgo::size_1D{ 0x100 },
				false
			);
		task->m_name = file;
		task->m_pos.m_main_id = pos;
		task->m_prepared = std::async
		(
			std::launch::async,
			[]
			(
				blob_queue_t* blob_queue,
				upload_task_t* task,
				std::filesystem::path path_local,
				upload_folder_t* parent
			)
			-> void
			{
				file_upload(blob_queue, task, path_local, parent);
				task->m_begin_progress = task->m_entity_arr.begin();
				task->m_end_progress = task->m_entity_arr.begin();
				return;
			},
			blob_queue,
			task.get(),
			file,
			&task->m_pos
		);
		task->m_self_i = load_state->m_task_list.emplace(load_state->m_task_list.end(), task);
	}
	return;
}

auto folder_upload_callback
(
	load_state_t* load_state,
	blob_queue_t* blob_queue,
	std::string pos
)
-> void
{
	std::vector<std::string> folder_arr{ dialog_file(true) };
	for (auto folder : folder_arr)
	{
		auto task{ std::make_shared<upload_task_t>() };
		task->m_state = load_state;
		task->m_type = "upload";
		std::uint64_t progress_div_id
		{
			load_state->m_progress_column->add_object<lgo::divy_flex_soft_t>
			(
				lgo::depth_range_t{ 0.0f, 0.5f },
				load_state->m_progress_column->space_x(lgo::std_margin),
				lgo::std_margin,
				lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::bottom },
				true
			)
		};
		task->m_progress_div = progress_div_id;
		load_state->m_progress_column->add_object<lgo::std_linex_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			load_state->m_progress_column->space_x(lgo::std_margin),
			lgo::std_white
		);
		task->m_progress_text =
			load_state->m_progress_column->get_object_shared<lgo::divy_flex_soft_t>(progress_div_id)->
				add_object_shared<lgo::text_line_scroll_std_t>
			(
				lgo::depth_range_t{ 0.0f, 0.5f },
				"資料夾：0B/?B " + folder,
				lgo::std_blue_light,
				lgo::size_1D{ 0x100 },
				false
			);
		task->m_name = folder;
		task->m_pos.m_main_id = pos;
		task->m_prepared = std::async
		(
			std::launch::async,
			[]
			(
				blob_queue_t* blob_queue,
				upload_task_t* task,
				std::filesystem::path path_local,
				upload_folder_t* parent
			)
			-> void
			{
				folder_upload(blob_queue, task, path_local, parent);
				task->m_begin_progress = task->m_entity_arr.begin();
				task->m_end_progress = task->m_entity_arr.begin();
				return;
			},
			blob_queue,
			task.get(),
			folder,
			&task->m_pos
		);
		task->m_self_i = load_state->m_task_list.emplace(load_state->m_task_list.end(), task);
	}
	return;
}

auto file_download_callback
(
	load_state_t* load_state,
	std::string name,
	std::string pos
)
-> void
{
	std::vector<std::string> folder{ dialog_file(true) };
	// Cancellation returns no paths; do not create a download task.
	if (folder.empty())
	{
		return;
	}
	if (folder.size() != 1)
	{
		throw;
	}
	auto task{ std::make_shared<download_task_t>() };
	task->m_state = load_state;
	task->m_type = "download";
	task->m_name = (std::filesystem::path{ folder[0] } / name).string();
	task->m_pos = pos;
	task->m_prepared = std::async
	(
		std::launch::async,
		[]
		(
			download_task_t* task,
			std::string pos,
			std::filesystem::path path_local
		)
		-> void
		{
            // get() is thread_local: acquire it in this worker, never on the GUI thread.
            std::filesystem::remove_all(path_local);
			file_download(&google_easy_t::get(), task, pos, path_local);
			return;
		},
		task.get(),
		pos,
		std::filesystem::path{ folder[0] } / name
	);
	task->m_progress_div =
		load_state->m_progress_column->add_object<lgo::divy_flex_soft_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			load_state->m_progress_column->space_x(lgo::std_margin),
			lgo::std_margin,
			lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::bottom },
			true
		);
	load_state->m_progress_column->add_object<lgo::std_linex_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		load_state->m_progress_column->space_x(lgo::std_margin),
		lgo::std_white
	);
	task->m_progress_text =
		load_state->m_progress_column->get_object_shared<lgo::divy_flex_soft_t>(task->m_progress_div)->
			add_object_shared<lgo::text_line_scroll_std_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			"準備下載檔案：" + name,
			lgo::std_green_light,
			lgo::size_1D{ 0x100 },
			false
		);
	task->m_self_i = load_state->m_task_list.emplace(load_state->m_task_list.end(), task);
	return;
}

auto folder_download_callback
(
	load_state_t* load_state,
	std::string name,
	std::string pos
)
-> void
{
	std::vector<std::string> folder{ dialog_file(true) };
	// Cancellation returns no paths; do not create a download task.
	if (folder.empty())
	{
		return;
	}
	if (folder.size() != 1)
	{
		throw;
	}
	auto task{ std::make_shared<download_task_t>() };
	task->m_state = load_state;
	task->m_type = "download";
	task->m_name = (std::filesystem::path{ folder[0] } / name).string();
	task->m_pos = pos;
	task->m_prepared = std::async
	(
		std::launch::async,
		[]
		(
			download_task_t* task,
			std::string pos,
			std::filesystem::path path_local
		)
		-> void
		{
            // get() is thread_local: acquire it in this worker, never on the GUI thread.
            std::filesystem::remove_all(path_local);
			folder_download(&google_easy_t::get(), task, pos, path_local);
			return;
		},
		task.get(),
		pos,
		std::filesystem::path{ folder[0] } / name
	);
	task->m_progress_div =
		load_state->m_progress_column->add_object<lgo::divy_flex_soft_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			load_state->m_progress_column->space_x(lgo::std_margin),
			lgo::std_margin,
			lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::bottom },
			true
		);
	load_state->m_progress_column->add_object<lgo::std_linex_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		load_state->m_progress_column->space_x(lgo::std_margin),
		lgo::std_white
	);
	task->m_progress_text =
		load_state->m_progress_column->get_object_shared<lgo::divy_flex_soft_t>(task->m_progress_div)->
			add_object_shared<lgo::text_line_scroll_std_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			"準備下載資料夾：" + name,
			lgo::std_green_light,
			lgo::size_1D{ 0x100 },
			false
		);
	task->m_self_i = load_state->m_task_list.emplace(load_state->m_task_list.end(), task);
	return;
}

auto delete_all_callback
(
	std::shared_ptr<lgo::divy_hard_t> center_div,
	std::shared_ptr<lgo::divy_soft_t> progress_bar_div,
	load_state_t* load_state
)
-> void
{
	/*auto input_div
	{
		center_div->add_object_shared<lgo::divy_flex_soft_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			lgo::size_1D{ 0x400 },
			lgo::std_margin,
			lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::center }
		)
	};
	input_div->add_object<lgo::text_line_flex_std_t>
	(
		lgo::depth_range_t{ 0.0f, 0.0f },
		"真的要清除的話輸入\"清除\"",
		lgo::std_sideways,
		lgo::std_red
	);
	auto focus{ new lgo::focus_t{} };
	auto input
	{
		input_div->add_object_shared<lgo::text_line_flex_std_input_t>
		(
			lgo::depth_range_t{ 0.0f, 0.0f },
			"",
			focus,
			lgo::std_sideways,
			lgo::std_red
		)
	};
	input_div->add_object<lgo::button_classic_std_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		std::bind(&delete_all_confirm_callback, center_div, progress_bar_div, input, uploading_list),
		"確認"
	);
	input_div->add_object<lgo::button_classic_std_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		std::bind(&delete_all_cancel_callback, center_div),
		"取消"
	);*///...
	return;
}

auto delete_all_confirm_callback
(
	std::shared_ptr<lgo::divy_hard_t> center_div,
	std::shared_ptr<lgo::divy_soft_t> progress_bar_div,
	std::shared_ptr<lgo::text_line_flex_std_input_t> input,
	load_state_t* load_state
)
-> void
{
	/*if (input->get_text() == "清除")
	{
		auto loading{ std::make_shared<loading_t>() };
		loading->m_type = "delete_all";
		loading->m_name = "清除";
		loading->m_progress_bar_id =
			progress_bar_div->add_object<lgo::divy_flex_soft_t>
			(
				lgo::depth_range_t{ 0.0f, 0.5f },
				progress_bar_div->space_x(lgo::std_margin),
				lgo::std_margin,
				lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::bottom },
				true
			);
		std::unique_lock lock{ uploading_list->m_mutex };
		uploading_list->m_list.push_back(loading);
		uploading_list->m_list_process.push_back(loading);
		lock.unlock();
	}
	center_div->clear_state();*///...
	return;
}

auto delete_all_cancel_callback(std::shared_ptr<lgo::divy_hard_t> center_div) -> void
{
	//center_div->clear_state();//...
	return;
}

auto folder_button_callback
(
	lgo::engine_t* engine,
	GUI_persistent_t* persistent,
	curl_multi_worker_t* multi,
	load_state_t* load_state,
	blob_queue_t* blob_queue,
	std::shared_ptr<lgo::divy_soft_t> viewer_div,
	std::string* pos_ptr,
	std::string pos
)
-> void
{
	*pos_ptr = pos;
	list
	(
		engine,
		persistent,
		multi,
		load_state,
		blob_queue,
		viewer_div,
		pos_ptr
	);
	return;
}

auto list
(
	lgo::engine_t* engine,
	GUI_persistent_t* persistent,
	curl_multi_worker_t* multi,
	load_state_t* load_state,
	blob_queue_t* blob_queue,
	std::shared_ptr<lgo::divy_soft_t> viewer_div,
	std::string* pos
)
-> void
{
	//Use `=` instead of `{}`: `nlohmann::json{ expr }` invokes the `std::initializer_list` constructor
	//and wraps the returned array into a 1-element outer array `[expr]`.
	nlohmann::json obj_list = list_folder_content(google_easy_t::get(), pos);
	viewer_div->clear_state();
	for (auto i : obj_list)
	{
		if (i["type"] == "folder")
		{
			viewer_div->add_object<lgo::button_text_t<lgo::text_line_scroll_std_t>>
			(
				lgo::depth_range_t{ 0.0f, 0.5f },
				std::bind
				(
					&folder_button_callback,
					engine,
					persistent,
					multi,
					load_state,
					blob_queue,
					viewer_div,
					pos,
					i["id"]
				),
				"資料夾，名稱：" + i["name"].get<std::string>(),
				lgo::std_white,
				lgo::size_1D{ viewer_div->space_2D(lgo::std_margin).x }
			);
			viewer_div->add_object<lgo::button_text_t<lgo::text_line_std_t>>
			(
				lgo::depth_range_t{ 0.0f, 0.5f },
				std::bind
				(
					&folder_download_callback,
					load_state,
					i["name"].get<std::string>(),
					i["id"].get<std::string>()
				),
				"下載",
				lgo::std_white,
				viewer_div->space_x(lgo::std_margin)
			);
			viewer_div->add_object<lgo::std_linex_t>
			(
				lgo::depth_range_t{ 0.0f, 0.5f },
				viewer_div->space_x(lgo::std_margin),
				lgo::std_white
			);
		}
		else if (i["type"] == "file")
		{
			viewer_div->add_object<lgo::button_text_t<lgo::text_line_scroll_std_t>>
			(
				lgo::depth_range_t{ 0.0f, 0.5f },
				[] () {},
				"檔案，名稱：" + i["name"].get<std::string>() + "，大小：" + std::to_string(i["size"].get<std::uint64_t>()),
				lgo::std_white,
				lgo::size_1D{ viewer_div->space_2D(lgo::std_margin).x }
			);
			viewer_div->add_object<lgo::button_text_t<lgo::text_line_std_t>>
			(
				lgo::depth_range_t{ 0.0f, 0.5f },
				std::bind
				(
					&file_download_callback,
					load_state,
					i["name"].get<std::string>(),
					i["id"].get<std::string>()
				),
				"下載",
				lgo::std_white,
				viewer_div->space_x(lgo::std_margin)
			);
			viewer_div->add_object<lgo::std_linex_t>
			(
				lgo::depth_range_t{ 0.0f, 0.5f },
				viewer_div->space_x(lgo::std_margin),
				lgo::std_white
			);
		}
		else
		{
			throw lgo::internal_error_t{ "obj type" };
		}
	}
	return;
}

auto home
(
	lgo::engine_t* engine,
	lgo::state_t* state,
	GUI_persistent_t* persistent,
	std::shared_ptr<lgo::divy_soft_t>* progress_column,
	load_state_t* load_state,
	curl_multi_worker_t* multi,
	blob_queue_t* blob_queue,
	std::string* pos
)
-> void
{
    state->clear_state();
    auto main_div
	{
		state->add_object_shared<lgo::divy_hard_t>
		(
			lgo::depth_range_t{ 0.0f, 1.0f },
			lgo::pos_2D{ 0, 0 },
			engine->get_window_size(),
			lgo::size_1D{ 0 },
			lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::top }
		)
	};

    auto menu_left
	{
		main_div->add_object_ghost_shared<lgo::divx_soft_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			lgo::div_create_from_member{},
			lgo::std_forward,
			lgo::std_margin,
			main_div->space_x(lgo::std_margin),
			lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::center }
		)
	};
    menu_left->add_object<lgo::text_line_std_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		"雲端硬碟",
		lgo::std_white,
		main_div->space_x(lgo::std_margin)
	);

    auto menu_right
	{
		main_div->add_object_shared<lgo::divx_soft_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			lgo::div_create_from_member{},
			lgo::std_forward,
			lgo::std_margin,
			main_div->space_x(lgo::std_margin),
			lgo::alignment_2D{ lgo::alignment_x::right, lgo::alignment_y::center }
		)
	};
    auto header_line = main_div->add_object_shared<lgo::std_linex_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		main_div->space_x(lgo::std_margin),
		lgo::std_white
	);
    auto work_div
	{
		main_div->add_object_shared<lgo::divx_soft_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			main_div->space_2D(lgo::std_margin),
			lgo::std_margin,
			lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::top }
		)
	};

    auto state_div
	{
		work_div->add_object_ghost_shared<lgo::divy_soft_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			lgo::size_2D{ 0x100, work_div->space_y(lgo::std_margin).x },
			lgo::std_margin,
			lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::top }
		)
	};

	persistent->m_frame_rate =
		state_div->add_object_shared<lgo::text_line_std_t>
		(
			lgo::depth_range_t{ 0.0f, 0.0f },
			"frame_rate: evaluating...",
			lgo::std_white,
			lgo::size_1D{ 0x100 }
		);
    // The main application loop pauses during interactive resizing; update FPS before each repaint.
    engine->callback_frame_rate_set
    (
        [frame_rate = std::weak_ptr{ persistent->m_frame_rate }](std::uint64_t rate)
        {
            if (auto label{ frame_rate.lock() })
            {
                auto text{ "frame_rate: " + std::to_string(rate) };
                if (label->get_text() != text)
                {
                    label->set_text(std::move(text));
                }
            }
        }
    );
	persistent->m_blob_queue_cycle_rate =
		state_div->add_object_shared<lgo::text_line_std_t>
		(
			lgo::depth_range_t{ 0.0f, 0.0f },
			"blob_queue_cycle_rate: evaluating...",
			lgo::std_white,
			lgo::size_1D{ 0x100 }
		);
	persistent->m_blob_queue_count =
		state_div->add_object_shared<lgo::text_line_std_t>
		(
			lgo::depth_range_t{ 0.0f, 0.0f },
			"blob_queue_count: evaluating...",
			lgo::std_white,
			lgo::size_1D{ 0x100 }
		);
	for (std::size_t i{ 0 }; i < load_easy_count / 4; ++i)
	{
		auto div
		{
			state_div->add_object_shared<lgo::divx_soft_t>
			(
				lgo::depth_range_t{ 0.0f, 0.5f },
				lgo::size_2D{ 0x100, 0x20 },
				lgo::std_margin,
				lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::top }
			)
		};
		for (std::size_t j{ 0 }; j < 4; ++j)
		{
			persistent->m_load_easy_status.push_back
			(
				div->add_object_shared<lgo::text_line_std_t>
				(
					lgo::depth_range_t{ 0.0f, 0.0f },
					"load_easy_status: evaluating...",
					lgo::std_white,
					lgo::size_1D{ 0x100 }
				)
			);
		}
	}
	
	*progress_column =
		work_div->add_object_shared<lgo::divy_soft_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			lgo::size_2D{ 0x100, work_div->space_y(lgo::std_margin).x },
			lgo::std_margin,
			lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::bottom },
			true
		);
	load_state->m_progress_column = *progress_column;
    auto column_line = work_div->add_object_shared<lgo::std_liney_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		work_div->space_y(lgo::std_margin),
		lgo::std_white
	);
    auto viewer_div
	{
		work_div->add_object_shared<lgo::divy_soft_t>
		(
			lgo::depth_range_t{ 0.0f, 0.5f },
			work_div->space_2D(lgo::std_margin),
			lgo::std_margin,
			lgo::alignment_2D{ lgo::alignment_x::left, lgo::alignment_y::top }
		)
	};

    load_state->m_on_upload_linked =
        [engine, persistent, multi, load_state, blob_queue, pos, viewer = std::weak_ptr{ viewer_div }]
        (const std::string& uploaded_pos)
        {
            //Navigation already reloads other folders. Refresh only the visible destination
            //so completing an upload never navigates the user away from their current folder.
            if (*pos == uploaded_pos)
            {
                if (auto viewer_div{ viewer.lock() })
                {
                    list(engine, persistent, multi, load_state, blob_queue, viewer_div, pos);
                }
            }
        };

	auto center_div
	{
		state->add_object_shared<lgo::divy_hard_t>
		(
			lgo::depth_range_t{ 0.0f, 1.0f },
			lgo::pos_2D{ 0, 0 },
			engine->get_window_size(),
			lgo::size_1D{ 0 },
			lgo::alignment_2D{ lgo::alignment_x::center, lgo::alignment_y::center }
		)
	};
	
    menu_right->add_object<lgo::button_classic_std_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		std::bind(&folder_create_callback, engine, state, pos),
		"新增資料夾"
	);
    menu_right->add_object<lgo::button_classic_std_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		[load_state, blob_queue, pos] { folder_upload_callback(load_state, blob_queue, *pos); },
		"上傳資料夾"
	);
    menu_right->add_object<lgo::button_classic_std_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		[load_state, blob_queue, pos] { file_upload_callback(load_state, blob_queue, *pos); },
		"上傳檔案"
	);
    menu_right->add_object<lgo::button_classic_std_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		std::bind(&delete_all_callback, center_div, *progress_column, load_state),
		"清除"
	);
    // Weak references keep the engine callback from retaining an obsolete UI after clear_state().
    engine->callback_window_resize_set
    (
        [main = std::weak_ptr{ main_div }, left = std::weak_ptr{ menu_left },
            right = std::weak_ptr{ menu_right }, header = std::weak_ptr{ header_line },
            work = std::weak_ptr{ work_div }, status = std::weak_ptr{ state_div },
            progress = std::weak_ptr{ *progress_column }, separator = std::weak_ptr{ column_line },
            viewer = std::weak_ptr{ viewer_div }, center = std::weak_ptr{ center_div }]
        (lgo::size_2D size)
        {
            auto main_div{ main.lock() };
            auto menu_left{ left.lock() };
            auto menu_right{ right.lock() };
            auto header_line{ header.lock() };
            auto work_div{ work.lock() };
            auto state_div{ status.lock() };
            auto progress_column{ progress.lock() };
            auto column_line{ separator.lock() };
            auto viewer_div{ viewer.lock() };
            auto center_div{ center.lock() };
            if (!main_div || !menu_left || !menu_right || !header_line || !work_div ||
                !state_div || !progress_column || !column_line || !viewer_div || !center_div)
            {
                return;
            }
            main_div->set_size(size);
            menu_left->set_size(lgo::size_2D{ main_div->space_x(lgo::std_margin).x, menu_left->get_size().y });
            menu_right->set_size(lgo::size_2D{ main_div->space_x(lgo::std_margin).x, menu_right->get_size().y });
            header_line->set_size(lgo::size_2D{ main_div->space_x(lgo::std_margin).x, header_line->get_size().y });
            // space_2D() reports unused space; add back the existing child along the flow axis.
            auto work_size{ main_div->space_2D(lgo::std_margin) };
            work_size.y += work_div->get_size().y;
            work_div->set_size(work_size);
            const auto height{ work_div->space_y(lgo::std_margin).x };
            state_div->set_size(lgo::size_2D{ state_div->get_size().x, height });
            progress_column->set_size(lgo::size_2D{ progress_column->get_size().x, height });
            column_line->set_size(lgo::size_2D{ column_line->get_size().x, height });
            auto viewer_size{ work_div->space_2D(lgo::std_margin) };
            viewer_size.x += viewer_div->get_size().x;
            viewer_div->set_size(viewer_size);
            viewer_div->fit_children_sideways();
            center_div->set_size(size);
        }
    );
    // Below this size the fixed-width desktop controls and status column would overlap.
    glfwSetWindowSizeLimits(engine->window_get(), 800, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);
	list
	(
		engine,
		persistent,
		multi,
		load_state,
		blob_queue,
		viewer_div,
		pos
	);
	return;
}
