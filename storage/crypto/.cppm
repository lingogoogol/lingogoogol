export module storage.crypto;

import std;

export
{
	auto sha256withrsa(std::string in, std::string key) -> std::string;
}
