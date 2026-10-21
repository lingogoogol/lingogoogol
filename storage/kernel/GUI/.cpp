module;

#include <nlohmann/json.hpp>

module storage.kernel.GUI;

import std;

import external.curl;

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
	curl_multi_t* multi,
	load_state_t* load_state,
	blob_queue_t* blob_queue
)
-> bool
{
	using namespace std::chrono_literals;
	
	//A array of easy handle that is not transfering and can be used.
	static std::vector<google_easy_t> easy_idle_arr( load_easy_count );
	static bool load_easy_status_init{ false };
	static std::map<CURL*, easy_status_t> load_easy_status_map{};
	if (!load_easy_status_init)
	{
		for (std::size_t i{ 0 }; i < easy_idle_arr.size(); ++i)
		{
			load_easy_status_map[easy_idle_arr[i].interface_get()].m_text_index = i;
		}
		load_easy_status_init = true;
	}

	persistent->m_frame_rate->set_text("frame_rate: " + std::to_string(engine->frame_rate()));
	persistent->m_blob_queue_cycle_rate->set_text("blob_queue_cycle_rate: " + std::to_string(blob_queue->cycle_rate()));
	persistent->m_blob_queue_count->set_text("blob_queue_count: " + std::to_string(blob_queue->count()));
	#ifdef _DEBUG
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
	#endif

	static std::chrono::steady_clock::time_point last_add_time{};
	std::chrono::steady_clock::time_point now{ std::chrono::steady_clock::now() };
	if (now - last_add_time > 0s && easy_idle_arr.size() && blob_queue->count() > 10000)
	{
		last_add_time = now;
		CURL* key{ easy_idle_arr.back().interface_get() };
		if (transfer_begin(load_state, *multi, std::move(easy_idle_arr.back()), blob_queue))
		{
			load_easy_status_map[key].m_add_time = std::chrono::steady_clock::now();
			easy_idle_arr.pop_back();
		}
	}
	multi->run();
	while (true)
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

auto dialog_file
(
	lgo::engine_t* engine,
	GUI_persistent_t* persistent,
	curl_multi_t* multi,
	load_state_t* load_state,
	blob_queue_t* blob_queue,
	bool folder
)
-> std::vector<std::string>
{
	dialog_t dialog{ folder };
	while (!dialog.ok())
	{
		message_loop(engine, persistent, multi, load_state, blob_queue);
	}
	return dialog.result();
}

auto folder_create_callback(lgo::engine_t* engine, lgo::state_t* state, std::string* pos) -> void
{
	return;
}

auto file_upload_callback
(
	lgo::engine_t* engine,
	GUI_persistent_t* persistent,
	curl_multi_t* multi,
	load_state_t* load_state,
	blob_queue_t* blob_queue,
	std::string pos
)
-> void
{
	std::vector<std::string> file_arr
	{
		dialog_file(engine, persistent, multi, load_state, blob_queue, false)
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
				google_easy_t* easy,
				blob_queue_t* blob_queue,
				upload_task_t* task,
				std::filesystem::path path_local,
				upload_folder_t* parent
			)
			-> void
			{
				file_upload(blob_queue, task, path_local, parent);
				auto file{ std::dynamic_pointer_cast<upload_file_t>(task->m_entity_arr.front()) };
				folder_add
				(
					*easy,
					task->m_pos.m_main_id,
					nlohmann::json
					{
						{ "id", file->m_main_id },
						{ "type", "file" },
						{ "name", std::filesystem::path{ file->m_path_local }.filename().string() },
						{ "size", file->m_size }
					}
				);
				task->m_begin_progress = task->m_entity_arr.begin();
				task->m_end_progress = task->m_entity_arr.begin();
				return;
			},
			&google_easy_t::get(),
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
	lgo::engine_t* engine,
	GUI_persistent_t* persistent,
	curl_multi_t* multi,
	load_state_t* load_state,
	blob_queue_t* blob_queue,
	std::string pos
)
-> void
{
	std::vector<std::string> folder_arr{ dialog_file(engine, persistent, multi, load_state, blob_queue, true) };
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
				google_easy_t* easy,
				blob_queue_t* blob_queue,
				upload_task_t* task,
				std::filesystem::path path_local,
				upload_folder_t* parent
			)
			-> void
			{
				folder_upload(blob_queue, task, path_local, parent);
				auto folder{ std::dynamic_pointer_cast<upload_folder_t>(task->m_entity_arr.front()) };
				folder_add
				(
					*easy,
					task->m_pos.m_main_id,
					nlohmann::json
					{
						{ "id", folder->m_main_id },
						{ "type", "folder" },
						{ "name", folder->m_name }
					}
				);
				task->m_begin_progress = task->m_entity_arr.begin();
				task->m_end_progress = task->m_entity_arr.begin();
				return;
			},
			&google_easy_t::get(),
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
	lgo::engine_t* engine,
	GUI_persistent_t* persistent,
	curl_multi_t* multi,
	load_state_t* load_state,
	blob_queue_t* blob_queue,
	std::string name,
	std::string pos
)
-> void
{
	std::vector<std::string> folder{ dialog_file(engine, persistent, multi, load_state, blob_queue, true) };
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
			google_easy_t* easy,
			download_task_t* task,
			std::string pos,
			std::filesystem::path path_local
		)
		-> void
		{
			std::filesystem::remove_all(path_local);
			file_download(easy, task, pos, path_local);
			task->m_idle_first = task->m_file_arr.begin();
			return;
		},
		&google_easy_t::get(),
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
			"檔案：0B/?B " + name,
			lgo::std_green_light,
			lgo::size_1D{ 0x100 },
			false
		);
	task->m_self_i = load_state->m_task_list.emplace(load_state->m_task_list.end(), task);
	return;
}

auto folder_download_callback
(
	lgo::engine_t* engine,
	GUI_persistent_t* persistent,
	curl_multi_t* multi,
	load_state_t* load_state,
	blob_queue_t* blob_queue,
	std::string name,
	std::string pos
)
-> void
{
	std::vector<std::string> folder{ dialog_file(engine, persistent, multi, load_state, blob_queue, true) };
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
			google_easy_t* easy,
			download_task_t* task,
			std::string pos,
			std::filesystem::path path_local
		)
		-> void
		{
			std::filesystem::remove_all(path_local);
			folder_download(easy, task, pos, path_local);
			task->m_idle_first = task->m_file_arr.begin();
			return;
		},
		&google_easy_t::get(),
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
			"資料夾：0B/?B " + name,
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
	curl_multi_t* multi,
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
	curl_multi_t* multi,
	load_state_t* load_state,
	blob_queue_t* blob_queue,
	std::shared_ptr<lgo::divy_soft_t> viewer_div,
	std::string* pos
)
-> void
{
	nlohmann::json obj_list{ list_folder_content(google_easy_t::get(), pos) };
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
					engine,
					persistent,
					multi,
					load_state,
					blob_queue,
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
					engine,
					persistent,
					multi,
					load_state,
					blob_queue,
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
	curl_multi_t* multi,
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
    main_div->add_object_shared<lgo::std_linex_t>
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
	#ifdef _DEBUG
	for (std::size_t i{ 0 }; i < load_easy_count; ++i)
	{
		persistent->m_load_easy_status.push_back
		(
			state_div->add_object_shared<lgo::text_line_std_t>
			(
				lgo::depth_range_t{ 0.0f, 0.0f },
				"load_easy_status: evaluating...",
				lgo::std_white,
				lgo::size_1D{ 0x100 }
			)
		);
	}
	#endif
	
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
    work_div->add_object<lgo::std_liney_t>
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
		std::bind(&folder_upload_callback, engine, persistent, multi, load_state, blob_queue, *pos),
		"上傳資料夾"
	);
    menu_right->add_object<lgo::button_classic_std_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		std::bind(&file_upload_callback, engine, persistent, multi, load_state, blob_queue, *pos),
		"上傳檔案"
	);
    menu_right->add_object<lgo::button_classic_std_t>
	(
		lgo::depth_range_t{ 0.0f, 0.5f },
		std::bind(&delete_all_callback, center_div, *progress_column, load_state),
		"清除"
	);
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
