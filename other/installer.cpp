#include <iostream>

auto http_request() -> void {
	lgo::tcp_client client{ U"example.com"_strmdc, U"50"_intpc };
	lgo::strmd8 send{
		U"GET / HTTP/1.1\r\n"
		U"Host: example.com\r\n"
		U"Content-Length: 0\r\n"
		U"\r\n"_strmd8
	};

	lgo::strmd8 header{ client.pad(send, lgo::seq_plain{}).get<lgo::strmd8>
	(lgo::io_until_ignore{ lgo::memmd{}.pad_resize(U"\r\n\r\n"_strmd8, lgo::seq_plain{}) }) + U"\r\n"_strmd8 };
	lgo::cmd.pad(header).pad(U"\n"_chac);

	lgo::intpc body_len{ lgo::get_http_header(header, U"Content-Length"_strmd8)
	.get<lgo::intpc>(lgo::int_base<lgo::intpc::val_v<U"A">>{}) };
	lgo::strmd8 body{ client.get<lgo::strmd8>(lgo::io_len{ body_len }) };
	lgo::cmd.pad(body).pad(U"\n"_chac);

	lgo::tcp_client https{ U"example.com"_strmdc, U"1BB"_intpc };

	//lgo::curve25519 key{};

	lgo::tls::extension server_name{
		lgo::ptrm<lgo::tls::server_name>{
			lgo::arrmd<lgo::arrmc<lgo::intp8, lgo::strmd8>>{
				lgo::seq_data{ lgo::arrmc<lgo::intp8, lgo::strmd8>{
					lgo::seq_data{ lgo::tls::name_type::host, U"example.com"_strmd8 }
				} }
			}
		}
	};
	lgo::tls::extension supported_versions{
		lgo::ptrm<lgo::tls::supported_versions>{
			lgo::arrmd<lgo::intp16>{
				lgo::seq_data{ lgo::tls::version::tls1_3 }
			}
		}
	};
	lgo::tls::extension supported_groups{
		lgo::ptrm<lgo::tls::supported_groups>{
			lgo::arrmd<lgo::intp16>{
				lgo::seq_data{ lgo::tls::group_type::x25519 }
			}
		}
	};
	lgo::tls::extension signature_algorithms{
		lgo::ptrm<lgo::tls::signature_algorithms>{
			lgo::arrmd<lgo::intp16>{
				lgo::seq_data{ lgo::tls::algorithm::rsa_pkcs1_sha256 }
			}
		}
	};
	/*lgo::tls::extension key_share_client_hello{
		lgo::ptrm<lgo::tls::key_share_client_hello>{
			lgo::arrmd<lgo::tls::group>{
				lgo::seq_data{
					lgo::tls::group{
						lgo::ptrm<lgo::tls::x25519>{
							key
						}
					}
				}
			}
		}
	};*/

	lgo::ptrm<lgo::tls::handshake> client_hello_handshake{
		lgo::ptrm<lgo::tls::client_hello>{
			lgo::tls::version::tls1_2,
			lgo::rand<lgo::numzs8<lgo::intpc::val_v<U"0">, lgo::intpc::val_v<U"20">>>(),
			lgo::memmd{},
			lgo::arrmd<lgo::intp16>{
				lgo::seq_data{ lgo::tls::cipher_suite::TLS_AES_128_GCM_SHA256 }
			},
			lgo::arrmd<lgo::intp8>{
				lgo::seq_data{ U"00"_intp8 }
			},
			lgo::arrmd<lgo::tls::extension>{
				lgo::seq_data{
					server_name.r(),
					supported_versions.r(),
					supported_groups.r(),
					signature_algorithms.r(),
					//key_share_client_hello.r()
				}
			}
		}
	};

	lgo::tls::record_plain client_hello{ lgo::tls::version::tls1_2, client_hello_handshake.r() };
	lgo::cmd.pad(lgo::memmd{}.pad_resize(client_hello)).pad(U"\n"_chac);
	https.pad(client_hello);

	lgo::memms<lgo::intpc::val_v<U"7">> alert{ https.get<lgo::memms<lgo::intpc::val_v<U"7">>>() };
	lgo::cmd.pad(alert).pad(U"\n"_chac);
	return;
}

const lgo::strmdc key_n{
	U"bff79264fa18751907a5061b0f2650a9e389075c4618609e29f8bc0175d84bc1"
	U"7ff2ab38f787a6c2174e63e901759d66d50cc1eaeabbbaed3695a1eac12b4019"
	U"c0319d616672d7625f75ba4668bdfae97fc4e623f23a1089bb16bab073b5a1be"
	U"5388d84bab56221d024a72fe398b9eab06b4be54faff41baf82f885441baf5d4"
	U"8966ef4bc81b94fc50f305affc088cf8f0304ed34dc7e7d6c8f4cdcc760f3a5d"
	U"9855b31a08227a6b35b83ba3d2b9fd65a77f1a5fe45ae23dd65e329bfb228ae8"
	U"71af6459e6c69dcfa45f17812ec0d41c317bbab4b7328f56e901af86a31bb342"
	U"37d6b0aca9a12459bb37b9860b89bb7b877c81ba450cb337c9624272a854e3c3"_strmdc
};

const lgo::strmdc key_d{
	U"002f9db93252d6be34f73077d33e6ec293615d1bf6476cc48a76debbc3a0f785"
	U"d2f969b80ee9573c85e842098642566a3497a41275b2c3de19f8f024b99630a4"
	U"35bef68f93195663168397301d46ddd8df125244c3a5d1c148689fbaaf0a0483"
	U"9a321e3f88857757feda129d7371d2b008bc211914fb2d55a08aef3f61a6ec39"
	U"0d90d95ba64f01eac647623d0e75798ba4877be7ea0a7a9da7a0151e025bc3d8"
	U"7b044f7d7a1cd962e821621d5e23977daeb9cdf0f47f1222beb6230984e0c918"
	U"c8b6646d1ff413a18883c60e98b81fe1aea2a376072cb5250013924a2245ee93"
	U"490f37ae4e134a761cc6f1d9e2e4b3340cd0504a21bf4f1868fc7e44b3204481"_strmdc
};

/*lgo::strd get_token() {
	std::time_t time{ std::time(nullptr) };
	auto encoded{ lgo::base64_encode<lgo::base64_type::url>(lgo::dmem{lgo::strd{
			"{"
			"  \"alg\":\"RS256\","
			"  \"typ\":\"JWT\""
			"}"
		}}) + "." + lgo::base64_encode<lgo::base64_type::url>(lgo::dmem{lgo::strd{
			"{"
			"  \"iss\":\"installer@lingogoogoltest.iam.gserviceaccount.com\","
			"  \"scope\":\"https://www.googleapis.com/auth/drive\","
			"  \"aud\":\"https://oauth2.googleapis.com/token\","
			"  \"exp\":" } + std::to_string(time + 3600).data() + ","
			"  \"iat\":" + std::to_string(time).data() +
			"}"
		}) };
	auto body{lgo::strd{
			"grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion="
		} + encoded + "." + lgo::base64_encode<lgo::base64_type::url>(
		lgo::rsassa_pkcs1_v1_5_sign(lgo::dmem<lgo::byte>{ encoded },
			lgo::dnum<lgo::qword>{ n }, lgo::dnum<lgo::qword>{ d }))
	};
	lgo::strd response_header{};
	lgo::strd response_body{};
	lgo::http_request("https://oauth2.googleapis.com/token",
		lgo::http_method::post, std::map<lgo::strd, lgo::strd>{}, body,
		response_header, response_body);
	return lgo::make_json(response_body)->get_val("access_token")->get_str();
}*/

/*std::pair<lgo::strd, lgo::strd> auth_header(const lgo::strd& token) {
	return { "Authorization", lgo::strd{ "Bearer " } + token };
}*/

/*lgo::strd create_file(const lgo::dmem& data, const lgo::strd& token) {
	const std::map<lgo::strd, lgo::strd> headers{
		auth_header(token),
		{ "Content-Length", "0" }
	};
	lgo::strd response_header{};
	lgo::strd response_body{};
	lgo::http_request(
		"https://www.googleapis.com/upload/drive/v3/files?uploadType=resumable",
		lgo::http_method::post, headers, lgo::dmem<lgo::byte>{},
		response_header, response_body);
	std::cout << response_header + response_body << '\n';
	lgo::strd location{ lgo::get_http_header(response_header, "Location") };
	response_header.resize(0);
	response_body.resize(0);
	std::cout << lgo::http_request(location, lgo::http_method::put,
		std::map<lgo::strd, lgo::strd>{ { "Content-Length", std::to_string(data.size()) } },
		data, response_header, response_body) << '\n';
	std::cout << response_header + response_body << '\n';
	return lgo::make_json(response_body)->get_val("id")->get_str();
}*/

/*void delete_file(const lgo::strd& id, const lgo::strd& token) {
	const std::map<lgo::strd, lgo::strd> headers{
		auth_header(token)
	};
	lgo::strd response_header{};
	lgo::strd response_body{};
	lgo::http_request("https://www.googleapis.com/drive/v3/files/" + id,
		lgo::http_method::del, headers,
		lgo::dmem<lgo::byte>{}, response_header, response_body);
}*/

/*lgo::json_pv* list_file(const lgo::strd& token) {
	const std::map<lgo::strd, lgo::strd> headers{
		auth_header(token)
	};
	lgo::strd response_header{};
	lgo::strd response_body{};
	lgo::http_request("https://www.googleapis.com/drive/v3/files",
		lgo::http_method::get, headers,
		lgo::dmem<lgo::byte>{}, response_header, response_body);
	std::cout << response_body << '\n';
	return lgo::make_json(response_body)->get_val("files");
}*/

int main() {
	http_request();
	/*std::string tmp{};
	std::cout << "輸入要刪除的檔案名稱：";
	std::getline(std::cin, tmp, '\n');
	lgo::strd to_del{ tmp };
	std::cout << "輸入要傳送的來源檔案名稱：";
	std::getline(std::cin, tmp, '\n');
	lgo::strd src_name{ tmp };
	std::cout << "輸入要傳送的目的檔案名稱：";
	std::getline(std::cin, tmp, '\n');
	lgo::strd dest_name{ tmp };
	std::cout << "計算OAuth令牌中…\n";
	lgo::strd token{ get_token() };
	lgo::json_pv* files{ list_file(token) };
	std::cout << "刪除檔案中…\n";
	for (lgo::intf64 i{ 0 }; i < files->len(); ++i) {
		std::cout << (*files)[i]->get_val("id")->get_str() << '\n';
		delete_file((*files)[i]->get_val("id")->get_str(), token);
	}
	while (true) {}
	std::ifstream in{ src_name };
	lgo::dmem data{};
	std::cout << "讀取檔案中…\n";
	while (true) {
		char ch{};
		in.get(ch);
		if (in.eof())
			break;
		data.push(ch);
	}
	std::cout << "上傳檔案中…\n";
	lgo::strd d{ create_file(data, token) };
	std::cout << d << '\n';*/
	lgo::cmd.pad(U"Complete!"_strmdc).pad(U"\n"_chac);
	while (true) {}
	return 0;
}