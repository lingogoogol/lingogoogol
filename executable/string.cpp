#include "string.h"

std::u32string to_u32string(std::string s8) {
	std::u32string s32{};
	for (int i{ 0 }; i < s8.size(); i++)
		s32.push_back(static_cast<char32_t>(s8[i]));
	return s32;
}

std::string to_string(std::u32string s32) {
	std::string s8{};
	for (int i{ 0 }; i < s32.size(); i++)
		s8.push_back(static_cast<char>(s32[i]));
	return s8;
}