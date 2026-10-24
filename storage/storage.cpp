#include <nlohmann/json.hpp>

import std;

import external.Windows;
import external.processenv;
import external.shellapi;

import lgo.math;
import lgo.io;
import lgo.GUI;

import storage.http;
import storage.google;
import storage.kernel;

std::atomic_bool close_app{ false };

std::string root_id{};

/*auto delete_all(loading_list_t* list, blob_queue_t* empty_blob) -> void
{
	if (obj_ptr->m_type == "delete_all")
	{
		auto progress_bar
		{
			progress_bar_div->add_object_shared<lgo::text_line_scroll_std_t>
			(
				lgo::depth_range_t{ 0.0f, 0.5f },
				"刪除：0/? " + obj_ptr->m_name,
				lgo::std_blue_light,
				lgo::size_1D{ 0x100 },
				false
			)
		};
		lock1.unlock();
		blob_update_name_run
		(
			google_easy_t::get(),
			root_id,
			nlohmann::json
			{
				{ "name", folder_root_name },
				{ "parent", "null" },
				{ "begin", "null" },
				{ "end", "null" },
				{ "count", 0 }
			}
			.dump()
		);
		google_easy_t::get().clear();
		google_easy_t::get().path_set("/drive/v3/files");
		empty_blob->clear_begin();
		nlohmann::json response = google_easy_t::get().method_get_run();
		while (true)
		{
			auto& array{ response["files"] };
			for (auto i : array)
			{
				if (i["id"] == root_id)
				{
					--obj_ptr->m_size;
					continue;
				}
				delete_file_t file{};
				file.m_id = i["id"];
				file.m_progress_bar = progress_bar;
				file.m_loading = obj_ptr;
				delete_id.push_back(file);
			}
			obj_ptr->m_size += array.size();
			if (!response.contains("nextPageToken"))
			{
				break;
			}
			google_easy_t::get().query_set("pageToken", response["nextPageToken"]);
			response = google_easy_t::get().method_get_run();
		}
		empty_blob->clear_end();
		progress_bar->set_text("刪除：0/" + std::to_string(obj_ptr->m_size) + " " + obj_ptr->m_name, false);
	}
	if (!delete_id.empty())
	{
		std::uint64_t delete_count{ std::min(delete_id.size(), 100ULL) };
		google_batch_t batch{ std::move(curl_easy_t::get()), "/drive/v3" };
		for (std::uint64_t i{ 0 }; i < delete_count; ++i)
		{
			http_request_t& request{ batch.request_add_handle() };
			request.method_set("DELETE");
			request.path_set("/drive/v3/files/" + delete_id[i].m_id);
		}
		batch.run();
		curl_easy_t::get() = std::move(batch);
		for (std::uint64_t i{ 0 }; i < delete_count; ++i)
		{
			++delete_id[i].m_loading->m_progress;
			delete_id[i].m_progress_bar->set_text
			(
				"刪除：" +
				std::to_string(delete_id[i].m_loading->m_progress) + "/" +
				std::to_string(delete_id[i].m_loading->m_size) + " " +
				delete_id[i].m_loading->m_name,
				false
			);
		}
		delete_id.erase(delete_id.begin(), delete_id.begin() + delete_count);
	}
	return;
}*/

auto app() -> void
{
	lgo::init_logfile("./logfile/" + std::to_string(std::time(nullptr)) + ".txt");
    lgo::log_file("CWD:\n");
	lgo::log_file("  " + std::filesystem::current_path().string() + "\n\n");
	lgo::FreeType_t freetype{};
	lgo::font_face_t::freetype_set(&freetype);
	lgo::font_face_t font_face{ "./font/NotoSansTC-VariableFont_wght.ttf", static_cast<std::uint64_t>(lgo::std_font_size.x) };
	lgo::font_preference_t::default_get().push_back(&font_face);
    lgo::engine_t engine{ lgo::size_2D{ 0x600, 0x400 }, "wWinMain.engine" };
    lgo::depth_tracker_t depth_tracker{};
    lgo::state_t state{ &engine, &depth_tracker };
	load_state_t load_state{};
    int argc{};
    CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc > 1)
	{
        lgo::log_file("參數數量不正確，不能超過0個\n");
    }
	//Use `=` instead of `{}`: `nlohmann::json{ expr }` invokes the `std::initializer_list` constructor
	//and wraps the parsed JSON object into a 1-element JSON array `[obj]`, which crashes `key["client_email"]`.
	nlohmann::json key = nlohmann::json::parse(lgo::get_file("key.json"));
	google_account_t::default_get().init
	(
		key["client_email"].get<std::string>(),
		key["private_key"].get<std::string>()
	);
	if (std::filesystem::exists("root"))
	{
		root_id = lgo::get_file_str("root");
	}
	else
	{
		root_id = folder_root_create(google_easy_t::get());
		lgo::create_ofstream_destroy("root") << root_id;
	}
	{
		blob_queue_t blob_queue{ "blob_queue", &google_account_t::default_get(), &close_app };
		std::string pos{ root_id };
		std::shared_ptr<lgo::divy_soft_t> progress_column{};
		GUI_persistent_t persistent{};
		home
		(
			&engine,
			&state,
			&persistent,
			&progress_column,
			&load_state,
			&curl_multi_t::get(),
			&blob_queue,
			&pos
		);
		while
		(
			!message_loop
			(
				&engine,
				&persistent,
				&curl_multi_t::get(),
				&load_state,
				&blob_queue
			)
		)
		{
			lgo::text_line_t::pending_load();
		}

		close_app = true;
		engine.flush();
        lgo::text_line_t::free();
	}
    state.clear_state();
	return;
}

auto __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) -> int
{
	try
	{
		app();
	}
	catch (const std::exception& e)
	{
		lgo::log_file(std::string{ "uncaught exception:\n" } + e.what());
	}
    lgo::log_file("finished_all!\n");
    return 0;
}
