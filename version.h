#ifndef VERSION_H
#define VERSION_H

#include <cstdint>
#include <string>
#include <array>
#include <locale>

#include "version_f.h"

template<int T1>
class Version {
public:
	Version();
	template<typename ...U1>
	Version(U1 ...args);
	std::uint_fast64_t& operator[](int index);
	const std::uint_fast64_t& operator[](int index) const;
	bool operator>(Version version);
	bool operator>=(Version version);
	bool operator==(Version version);
	bool operator<=(Version version);
	bool operator<(Version version);
	bool operator!=(Version version);
	template<int U1>
	friend std::fstream& operator<<(std::fstream& fs, const Version<U1>& version);
	template<int U1>
	friend std::fstream& operator>>(std::fstream& fs, Version<U1>& version);
	std::string to_string() const;
private:
	std::array<std::uint_fast64_t, T1> data{};
};

template<typename ...U1>
Version(U1...)->Version<sizeof...(U1)>;

template<int T1>
Version<T1>::Version() :data{} {
	for (int i{ 0 }; i < 5; i++)
		data[i] = std::uint_fast64_t{};
	return;
}

template<int T1>
template<typename ...U1>
Version<T1>::Version(U1 ...args) : data{} {
	int i{ 0 };
	((data[i++] = args), ...);
	return;
}

template<int T1>
std::uint_fast64_t& Version<T1>::operator[](int index) {
	return data[index];
}

template<int T1>
const std::uint_fast64_t& Version<T1>::operator[](int index) const {
	return data[index];
}

template<int T1>
bool Version<T1>::operator>(Version<T1> version) {
	for (int i{ 0 }; i < 5; i++)
		if (data[i] > version[i])
			return true;
		else if (data[i] < version[i])
			return false;
	return false;
}

template<int T1>
bool Version<T1>::operator>=(Version<T1> version) {
	for (int i{ 0 }; i < 5; i++)
		if (data[i] > version[i])
			return true;
		else if (data[i] < version[i])
			return false;
	return true;
}

template<int T1>
bool Version<T1>::operator==(Version<T1> version) {
	for (int i{ 0 }; i < 5; i++)
		if (data[i] != version[i])
			return false;
	return true;
}

template<int T1>
bool Version<T1>::operator<=(Version<T1> version) {
	for (int i{ 0 }; i < 5; i++)
		if (data[i] > version[i])
			return false;
		else if (data[i] < version[i])
			return true;
	return true;
}

template<int T1>
bool Version<T1>::operator<(Version<T1> version) {
	for (int i{ 0 }; i < 5; i++)
		if (data[i] > version[i])
			return false;
		else if (data[i] < version[i])
			return true;
	return false;
}

template<int T1>
bool Version<T1>::operator!=(Version<T1> version) {
	for (int i{ 0 }; i < 5; i++)
		if (data[i] != version[i])
			return true;
	return false;
}

template<int U1>
std::fstream& operator<<(std::fstream& fs, const Version<U1>& version) {
	int size_difference{ static_cast<int>(sizeof(std::uint_fast64_t)) - 8 };
	for (int i{ 0 }; i < U1; i++) {
		std::uint_fast64_t output{ version[i] << size_difference };
		fs.write((char*)&output, 8);
	}
	return fs;
}

template<int U1>
std::fstream& operator>>(std::fstream& fs, Version<U1>& version) {
	int size_difference{ static_cast<int>(sizeof(std::uint_fast64_t)) - 8 };
	for (int i{ 0 }; i < U1; i++) {
		std::uint_fast64_t input{};
		fs.read((char*)&input, 8);
		input >>= size_difference;
		version[i] = input;
	}
	return fs;
}

template<int T1>
std::string Version<T1>::to_string() const {
	std::string s{};
	for (int i{ 0 }; i < T1; i++) {
		s.insert(s.size(), std::to_string(data[i]));
		s.push_back('.');
	}
	s.pop_back();
	return s;
}

#endif