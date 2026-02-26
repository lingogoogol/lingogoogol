#ifndef VECTOR
#define VECTOR

#include <vector>
#include <string>
#include <cstdlib>

template<typename t_type>
std::vector<t_type> operator+(std::vector<t_type> front, const t_type& end) {
	front.push_back(end);
	return front;
}

template<typename t_type>
std::vector<t_type> operator+(std::vector<t_type> first, const std::vector<t_type>& second) {
	first.insert(first.end(), second.begin(), second.end());
	return first;
}

std::vector<std::string> split(std::string in, std::string delimiter) {
	std::vector<std::string> out{};
	std::size_t start{ 0 };
	std::size_t end{ 0 };
	while ((end = in.find(delimiter, start)) != std::string::npos) {
		out.push_back(in.substr(start, end - start));
		start = end + delimiter.size();
	}
	out.push_back(in.substr(start, in.size() - start));
	return out;
}

template<typename T1, typename T2>
auto map(const std::vector<T1>& in, const T2& process) {
	std::vector<decltype(process(in[0]))> out{};
	for (int i{ 0 }; i < in.size(); ++i) {
		out.push_back(process(in[i]));
	}
	return out;
}

template<typename T1, typename T2>
std::vector<T1> filter(const std::vector<T1>& in, const T2& cond) {
	std::vector<T1> out{};
	std::copy_if(in.begin(), in.end(), std::back_inserter(out), cond);
	return out;
}

auto difference(const std::vector<std::string>& in1, const std::vector<std::string>& in2) -> std::vector<std::string> {
    std::vector<std::string> out{};
    for (int i{ 0 }; i < in1.size(); ++i) {
        bool found{ false };
        for (int j{ 0 }; j < in2.size(); ++j) {
            if (in1[i] == in2[j]) {
                found = true;
                break;
            }
        }
        if (!found) {
            out.push_back(in1[i]);
        }
    }
    return out;
}

#endif