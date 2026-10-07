export module storage.encode;

import std;

export
{
	auto base64url_encode(std::string in) -> std::string;
	auto base64url_decode(std::string in) -> std::string;
}
