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
		nlohmann::json response{ google_easy_t::get().method_get_run() };
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
	lgo::font_face_t font_face{ "./font/NotoSansTC-VariableFont_wght.ttf", 12 };
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
	google_account_t::default_get().init
	(
		"public@lingogoogol.iam.gserviceaccount.com",
		"-----BEGIN PRIVATE KEY-----\n"
		"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDhuOYPiqBJLtRf\n"
		"pB1z8JHFFcWf14NsUrsdzd4ZrM3T9cbC3KD5bO0rwDGOFfuwEXx1miQtKD1R9nOe\n"
		"EUetHH8oUA0sOtR7SJe+1OTou/zHLK4YckjWP6HGktMKcqT650i3kpUCyBu0eB2z\n"
		"r8rFfk6PW1cEkTe9c2e07X0V5HG8dEZ43P3bueVgNamoP5s1N9hdKFfszLlU3zt1\n"
		"to2gxizPRCWDqOx8hIn478c+GCFkpvfmkkSqMDdlHe3hJyxwCAd/E73HORnAk7ww\n"
		"GDXi24S5jZDOm3kpNK6uXSaoWSOFYbduJY6x7s48nM1F+45rdP4IOerHOOGuRNft\n"
		"CBTXIZvZAgMBAAECggEABk3B3Qq/zbNpWm9spCWCiKjCP2PHJ5IK6Drs2jY1fFcn\n"
		"kR8UvsqAOCHSsHdkM0dVvLoMixdN2RljIrW0VP0P/wn3SDYvY28ET9/Fscs4NL7G\n"
		"cOfWKWeXBsLocmj/uwMVCI1nLG/2uBxHphJBfTD5FC9Wl4nPrvKHYanlPaSG2Z6v\n"
		"PoYfMAXCFY7oIBVlbZ935HfbwoWL0DIMlruxRZ6b82bsRvnYLhTCuKKapr7V0gwT\n"
		"YklBKEcyGznNCmoe+7669JwfFFt8Y7JMVbGurx7ulBBztC/5OckbmQrs+un5iuZC\n"
		"+kwlHjZKHtmTE826u2Joks834xxDYZjR5vJL3lRP4QKBgQD5i1oRf92LuoRF5ZZK\n"
		"bglq2H+wXmUEa0lve/hcQsyJvZzA0ZfB9zgwBgrwu4zcmMiT/3Nt3rJkpCG9Bi0h\n"
		"OJJ+uYNItNs9o8IlohnnLv0clUzh38zrDFSrOPrJd9KJAKeEl1xZBZFJ9abNACaT\n"
		"bGzjPrfzipMGNTVNBWQN0SIEqQKBgQDnj8f9ggSwLvppdiYLXTVHhFgOe8tac59o\n"
		"fMNm/8nKgtzVjRbGLFkkbXIjWv9IcxCULYbFhCR/Fi4NgfX3g3MO90cjWs+cYCQA\n"
		"yck+QqBhf4yzdYxff9KRMoW2n/mMlXgpUckvv642CNSm4HyueHSdrS2GmCNJ0PEe\n"
		"4v9I2bQrsQKBgFeCHJWiZ0iGobf5KNj0+Wx0e2KX14jTLq9fo9qGuj9bNMsRNMya\n"
		"KyP8Gn4M5vq8wSnVHN9chuZwADzPG6hYtFaoK3elpHRbJswXqkm92pjZalqTJsMO\n"
		"UeqxUpCv6i6NAjGm+oVlRGvRLysYcG6mSRDvf4ZyA9jbc9OMnmeR/QHxAoGBAOO8\n"
		"Lo4N/xpfOXva7NOkWa/6Yrfr3DUbHZ2fhnWX/xeh2BpWZh2EpC8FTImlBwhz0cxJ\n"
		"chBzQ4mRgL1cIVHJ/7CqVVZaC3DZxM7ZpYgL8ruwu+oLI4EQr0CJlaPkdYNjxQeu\n"
		"HdVOCMGvsaRlh3n3kYk5Nlr26U3BpkIgMb2uDlohAoGBAOTI0j0YADYxCzUMOStD\n"
		"5IYy4ws33DUPu57NxoWVZRGaiPVAv/OmYdR++cZEAKeRNFMBLcJpKj1AFMVRAeyP\n"
		"2GVlqjPq81CsAClh+GZgRHwHyEGOo4OxecVUN7f2jW8p1tbPbc0Hd0EXfOoBwsKS\n"
		"WHB8uTlWqeZxM5LdFLOpGNr1\n"
		"-----END PRIVATE KEY-----\n"
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
