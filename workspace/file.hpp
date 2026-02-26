#ifndef WORKSPACE_FILE
#define WORKSPACE_FILE

#include <string>
#include <map>
#include <filesystem>
#include <exception>

#include "../compilercpp/lib/file.hpp"

auto file_to_string(std::filesystem::path path) -> std::string {
    auto ifstream{ create_ifstream(path.string()) };
    std::string out{};
    try {
        while (true) {
            out.push_back('\0');
            ifstream.get(out[out.size() - 1]);
        }
    }
    catch (std::exception e) {}
    return out;
}

auto CSV_to_map(std::filesystem::path path) -> std::map<std::string, std::string> {
    std::string file{ file_to_string(path) };
    std::size_t begin{ 0 }, end{};
    std::string key{}, val{};
    std::map<std::string, std::string> out{};
    while (true) {
        end = file.find(',', begin);
        if (end == std::string::npos) {
            break;
        }
        key = file.substr(begin, end - begin);
        begin = end + 1;
        end = file.find("\r\n", begin);
        if (end == std::string::npos) {
            break;
        }
        val = file.substr(begin, end - begin);
        begin = end + 2;
        out[key] = val;
    }
    return out;
}

#endif